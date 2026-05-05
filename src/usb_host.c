/**
 * @file usb_host.c
 * @brief TinyUSB HID ホストモジュール（Phase 3 実装）
 *
 * Phase 3 実装範囲:
 *   - tuh_init() による TinyUSB ホストスタック初期化
 *   - tuh_task() によるメインループ処理
 *   - HID_ITF_PROTOCOL_KEYBOARD のみマウント受け入れ（マウス等は無視）
 *   - Boot Protocol 強制（CFG_TUH_HID=4, boot keyboard 想定）
 *   - 接続/切断時に UART ログ出力 + LED 状態遷移
 *   - 切断時に keyrepeat_cancel() を呼び、リピートをキャンセル
 *   - tuh_hid_report_received_cb() は受信チェーン継続のみ（レポート解析は Phase 4）
 *
 * 将来対応 (Phase 4〜):
 *   - tuh_hid_report_received_cb() でのキーダウン/キーアップ検出・変換・UART送信
 *   - キーリピート登録 (keyrepeat_register / keyrepeat_cancel)
 *   - s_prev_report[] を用いた前回レポートとの差分検出
 *
 * 異常系 (Phase 7):
 *   - 過電流・通信エラーリカバリ・複数キーボード対応
 *   - tuh_init() 失敗時のセーフモード（tuh_task() 抑止） → Phase 7
 *   - tuh_hid_receive_report() 失敗時の再試行ロジック → Phase 7
 *   - Report Protocol のみ対応キーボード（Boot Protocol 非対応）への対応 → Phase 7 以降
 */

#include "usb_host.h"
#include "uart_out.h"
#include "led.h"
#include "keyrepeat.h"
#include "tusb.h"

#include <stdio.h>
#include <string.h>

/* ---- 内部状態 ---- */

static bool    s_keyboard_mounted = false;
static uint8_t s_kb_dev_addr      = 0;
static uint8_t s_kb_instance      = 0;
static uint8_t s_prev_report[8]   = {0}; /* 前回 HID レポート（Phase 4 で使用） */

/* ---- 公開 API ---- */

/**
 * @brief TinyUSB ホストスタックを初期化する
 *
 * BOARD_TUH_RHPORT (= 0) を使用して tuh_init() を呼び出す。
 * 初期化に失敗した場合は UART にエラーメッセージを出力し、
 * LED を LED_STATE_ERROR に設定する。
 */
void usb_host_init(void) {
    bool ok = tuh_init(BOARD_TUH_RHPORT);
    if (!ok) {
        uart_out_puts("[USB] tuh_init failed\r\n");
        led_set_state(LED_STATE_ERROR);
        /* TODO (Phase 7): tuh_init() 失敗時のセーフモード（tuh_task() 抑止）を実装する */
        return;
    }
    uart_out_puts("[USB] TinyUSB host initialized\r\n");
    led_set_state(LED_STATE_WAIT_DEVICE);
}

/**
 * @brief TinyUSB ホストタスクを処理する（メインループから定期的に呼び出す）
 */
void usb_host_task(void) {
    tuh_task();
}

/**
 * @brief キーボードのマウント状態を返す
 *
 * @return true: キーボード接続済み, false: 未接続
 */
bool usb_host_is_keyboard_mounted(void) {
    return s_keyboard_mounted;
}

/* ---- TinyUSB HID コールバック実装 ---- */

/**
 * @brief HIDデバイスマウント時コールバック
 *
 * TinyUSB がデバイスを列挙し HID インターフェースを検出したときに呼ばれる。
 * HID_ITF_PROTOCOL_KEYBOARD の場合のみ受け入れる。
 * キーボード以外（マウス等）は無視して即リターンする（設計書 8.3節）。
 * 複数 HID キーボード接続は Phase 7 で対応。本 Phase は最初の 1 個のみを追跡する。
 *
 * @param dev_addr    デバイスアドレス
 * @param instance    HID インスタンス番号
 * @param desc_report HID レポートデスクリプタ（Phase 3 では使用しない）
 * @param desc_len    レポートデスクリプタ長
 */
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const *desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    if (itf_protocol != HID_ITF_PROTOCOL_KEYBOARD) {
        /* キーボード以外（マウス等）は無視（設計書 8.3節） */
        char buf[64];
        snprintf(buf, sizeof(buf),
                 "[USB] Non-keyboard HID ignored (proto=%u)\r\n",
                 (unsigned)itf_protocol);
        uart_out_puts(buf);
        return;
    }

    /* 複数 HID キーボード接続は Phase 7 で対応。本 Phase は最初の 1 個のみを追跡する */
    if (s_keyboard_mounted) {
        return;
    }

    /* TinyUSB がエニュメレーション中に既に Boot Protocol を設定済みのため再設定不要。
     * Report Protocol のみ対応キーボード（Boot Protocol 非対応）への対応は Phase 7 以降の課題。
     */

    s_keyboard_mounted = true;
    s_kb_dev_addr      = dev_addr;
    s_kb_instance      = instance;
    memset(s_prev_report, 0, sizeof(s_prev_report)); /* 前回レポートをリセット */

    char buf[80];
    snprintf(buf, sizeof(buf),
             "[USB] Keyboard connected (addr=%u, instance=%u)\r\n",
             (unsigned)dev_addr, (unsigned)instance);
    uart_out_puts(buf);

    led_set_state(LED_STATE_MOUNTED);

    /* 最初のレポート受信を要求（受信チェーン開始） */
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        uart_out_puts("[USB] Failed to request HID report\r\n");
        /* TODO (Phase 7): tuh_hid_receive_report() 失敗時の再試行ロジックを実装する */
    }
}

/**
 * @brief HIDデバイスアンマウント時コールバック
 *
 * 追跡中のキーボードが切断されたときに状態をリセットし、
 * キーリピートをキャンセルして LED を待機状態に戻す（設計書 6.5節、8.2節）。
 * 追跡していない HID デバイス（マウス等）のアンマウントは無視する。
 *
 * @param dev_addr  デバイスアドレス
 * @param instance  HID インスタンス番号
 */
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    if (!s_keyboard_mounted ||
        dev_addr != s_kb_dev_addr ||
        instance != s_kb_instance) {
        /* 追跡していない HID デバイス（無視したマウス等）のアンマウントは無視 */
        return;
    }

    s_keyboard_mounted = false;
    s_kb_dev_addr      = 0;
    s_kb_instance      = 0;
    memset(s_prev_report, 0, sizeof(s_prev_report));

    uart_out_puts("[USB] Keyboard disconnected\r\n");

    /* キーリピートをキャンセル（設計書 6.5節） */
    keyrepeat_cancel();

    led_set_state(LED_STATE_WAIT_DEVICE);
}

/**
 * @brief HIDレポート受信コールバック
 *
 * Phase 3 では受信チェーンを継続するためのみ機能させる。
 * レポート内容の解析（キーダウン/キーアップ検出・keymap変換・UART送信・keyrepeat登録）
 * は Phase 4 で実装する。
 *
 * 受信チェーン: このコールバック内で tuh_hid_receive_report() を再呼び出しすることで
 * 次のレポートの受信を要求し続ける。
 *
 * @param dev_addr  デバイスアドレス
 * @param instance  HID インスタンス番号
 * @param report    受信レポートデータ（Phase 4 で使用）
 * @param len       受信レポート長（Phase 4 で使用）
 */
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                 uint8_t const *report, uint16_t len) {
    /* Phase 4 実装予定: レポート解析・キーダウン検出・keymap変換・UART送信・keyrepeat登録 */
    (void)report;
    (void)len;

    /* 受信チェーン継続: 次のレポート受信を要求 */
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        uart_out_puts("[USB] Failed to re-request HID report\r\n");
        /* TODO (Phase 7): tuh_hid_receive_report() 失敗時の再試行ロジックを実装する */
    }
}
