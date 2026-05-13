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
 * Phase 4 実装範囲（追加）:
 *   - tuh_hid_report_received_cb() でのレポート解析・差分検出・keymap変換・UART送信
 *
 * Phase 5 実装範囲（追加・keymap.c 側）:
 *   - keymap.c がテーブル方式（normal/shift/ctrl）にリライトされ、
 *     Shift/Ctrl 修飾キー、特殊キー（Esc/Tab/Space/BS）、記号キー、Keypad 数字を完全対応
 *   - usb_host.c は変更なし（既存の keymap_convert(usage, modifier) 呼び出しで自動対応）
 *
 * Phase 6 実装範囲（追加）:
 *   - キーダウン時に keyrepeat_register() でリピート対象キーを登録
 *   - 全キーリリース時に keyrepeat_cancel() を呼び、リピート停止
 *   - 送信のたびに led_set_state(LED_STATE_TX) で LED 短時間点滅
 *
 * 異常系 (Phase 7):
 *   - 過電流・通信エラーリカバリ・複数キーボード対応
 *   - tuh_init() 失敗時のセーフモード（tuh_task() 抑止） → Phase 7
 *   - tuh_hid_receive_report() 失敗時の再試行ロジック → Phase 7
 *   - Report Protocol のみ対応キーボード（Boot Protocol 非対応）への対応 → Phase 7 以降
 *
 * Phase 9 実装範囲（追加）:
 *   - 全ログ出力を uart_out_puts (UART0/SBC 側) から uart_monitor_puts (UART1/モニタ側) に振替
 *   - キー入力送信時、JP5 が SHORT (ON) なら UART1 にもローカルエコー出力
 *   - 既存の UART0 への送信動作（FR-006、FR-004）は変更なし
 */

#include "usb_host.h"
#include "uart_out.h"
#include "uart_monitor.h"
#include "config.h"
#include "led.h"
#include "keyrepeat.h"
#include "keymap.h"
#include "tusb.h"

#include <stdio.h>
#include <string.h>

/* HID Boot Keyboard レポートサイズ（仕様固定 8 バイト） */
#define BOOT_KB_REPORT_SIZE 8

/* ---- 内部状態 ---- */

static bool    s_keyboard_mounted = false;
static uint8_t s_kb_dev_addr      = 0;
static uint8_t s_kb_instance      = 0;
static uint8_t s_prev_report[BOOT_KB_REPORT_SIZE] = {0}; /* 前回 HID レポート（Phase 4 で使用） */

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
        uart_monitor_puts("[USB] tuh_init failed\r\n");
        led_set_state(LED_STATE_ERROR);
        /* TODO (Phase 7): tuh_init() 失敗時のセーフモード（tuh_task() 抑止）を実装する */
        return;
    }
    uart_monitor_puts("[USB] TinyUSB host initialized\r\n");
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
        uart_monitor_puts(buf);
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
    memset(s_prev_report, 0, BOOT_KB_REPORT_SIZE); /* 前回レポートをリセット */

    char buf[80];
    snprintf(buf, sizeof(buf),
             "[USB] Keyboard connected (addr=%u, instance=%u)\r\n",
             (unsigned)dev_addr, (unsigned)instance);
    uart_monitor_puts(buf);

    led_set_state(LED_STATE_MOUNTED);

    /* 最初のレポート受信を要求（受信チェーン開始） */
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        uart_monitor_puts("[USB] Failed to request HID report\r\n");
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
    memset(s_prev_report, 0, BOOT_KB_REPORT_SIZE);

    uart_monitor_puts("[USB] Keyboard disconnected\r\n");

    /* キーリピートをキャンセル（設計書 6.5節） */
    keyrepeat_cancel();

    led_set_state(LED_STATE_WAIT_DEVICE);
}

/**
 * @brief HIDレポート受信コールバック（Phase 4 実装）
 *
 * HID Boot Keyboard レポート（8 バイト固定）を解析し、新たに押されたキー
 * （前回レポートに存在しなかった Usage ID）を keymap で ASCII に変換し
 * UART に送信する。Enter キーはジャンパー設定の行末コードを送信する。
 *
 * Boot Protocol レポート形式:
 *   Byte 0    : Modifier byte（Phase 5 で Shift/Ctrl 解釈に使用）
 *   Byte 1    : Reserved（無視）
 *   Byte 2-7  : 同時押し最大 6 キーの HID Usage ID
 *
 * 差分検出:
 *   現在レポートのキースロットを走査し、前回レポートに存在しなかった
 *   Usage ID のみ「キーダウン」イベントとして処理する。
 *   これにより同じキーを押し続けても 1 文字のみ送信される。
 *
 * Phase 6 で追加済み:
 *   - keyrepeat_register() でリピート対象キーを登録
 *   - 全キーリリース時に keyrepeat_cancel() を呼びリピート停止
 *   - led_set_state(LED_STATE_TX) で送信時の LED 短時間点滅
 *
 * @param dev_addr  デバイスアドレス
 * @param instance  HID インスタンス番号
 * @param report    受信レポートデータ（Boot Keyboard: 8 バイト）
 * @param len       受信レポート長（Boot Keyboard: 8）
 */
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                 uint8_t const *report, uint16_t len) {
    if (len >= BOOT_KB_REPORT_SIZE && report != NULL) {
        /* Phantom 状態検出: Byte 2-7 に ErrorRollOver(0x01)/POSTFail(0x02)/ErrorUndefined(0x03)
         * のみが存在し、有効キー(>=0x04)が 1 つも無い場合は前回状態を保護する。
         * 「全スロット 0」は「全キー離した」状態なので Phantom 扱いせず通常処理に回す。
         * Phase 7 で異常系として詳細対応する余地あり。 */
        bool has_valid_key = false;
        bool has_phantom   = false;
        for (int i = 2; i < BOOT_KB_REPORT_SIZE; i++) {
            uint8_t const u = report[i];
            if (u >= 0x04) {
                has_valid_key = true;
            } else if (u >= 0x01 && u <= 0x03) {
                has_phantom = true;
            }
        }
        if (has_phantom && !has_valid_key) {
            /* Phantom only（有効キーなし、Phantom コードあり）: 状態保護して受信チェーン継続 */
            if (!tuh_hid_receive_report(dev_addr, instance)) {
                uart_monitor_puts("[USB] Failed to re-request HID report\r\n");
            }
            return;
        }

        uint8_t const modifier = report[0];

        /* Byte 2-7 のキースロットを走査し、新たに押されたキーのみ処理 */
        for (int i = 2; i < BOOT_KB_REPORT_SIZE; i++) {
            uint8_t const usage = report[i];
            if (usage == 0) {
                continue; /* 空スロット */
            }
            /* HID Boot Keyboard の Phantom/Error 状態（0x01: ErrorRollOver, 0x02: POSTFail,
             * 0x03: ErrorUndefined）はキー入力ではないためスキップ。
             * 6キー超過時に Byte 2-7 が 0x01 で埋まる仕様への対応。 */
            if (usage <= 0x03) {
                continue;
            }

            /* 前回レポートに同じ usage が含まれていれば押しっぱなし → 無視 */
            bool was_pressed = false;
            for (int j = 2; j < BOOT_KB_REPORT_SIZE; j++) {
                if (s_prev_report[j] == usage) {
                    was_pressed = true;
                    break;
                }
            }
            if (was_pressed) {
                continue;
            }

            /* キーダウンイベント */
            if (keymap_is_enter(usage)) {
                uart_out_send_line_ending();
                led_set_state(LED_STATE_TX);
                /* Enter は行末コードを 1 回送信のみ。リピート対象としない */
                /* Phase 9: JP5 ON ならモニタ側にも行末コードをローカルエコー */
                if (config_get_local_echo()) {
                    uart_monitor_send_line_ending();
                }
            } else {
                uint8_t const ascii = keymap_convert(usage, modifier);
                if (ascii != 0) {
                    uart_out_putc(ascii);
                    led_set_state(LED_STATE_TX);
                    keyrepeat_register(ascii);  /* Phase 6: キーリピート対象として登録 */
                    /* Phase 9: JP5 ON ならモニタ側にもローカルエコー */
                    if (config_get_local_echo()) {
                        uart_monitor_putc(ascii);
                    }
                }
            }
        }

        /* Phase 6: 全キーリリース検出 → リピート停止
         * 現在レポートの Byte 2-7 にリピート対象キー（英字・数字・記号・特殊キー）が
         * 1 つも無ければ、キーリピートをキャンセルする。
         * Enter（0x28）/ Keypad Enter（0x58）はリピート対象外なので除外。
         * Phantom 状態（0x01-0x03）も除外。 */
        bool any_repeatable_key = false;
        for (int i = 2; i < BOOT_KB_REPORT_SIZE; i++) {
            uint8_t const u = report[i];
            if (u >= 0x04 && !keymap_is_enter(u)) {
                any_repeatable_key = true;
                break;
            }
        }
        if (!any_repeatable_key) {
            keyrepeat_cancel();
        }

        /* 現在レポートを保存（次回の差分検出に使用） */
        memcpy(s_prev_report, report, BOOT_KB_REPORT_SIZE);
    }
    /* len < BOOT_KB_REPORT_SIZE または report == NULL の場合: Boot Protocol 想定外サイズ。
     * レポート保存は行わず受信チェーンだけ継続する。 */

    /* 受信チェーン継続: 次のレポート受信を要求 */
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        uart_monitor_puts("[USB] Failed to re-request HID report\r\n");
        /* TODO (Phase 7): tuh_hid_receive_report() 失敗時の再試行ロジックを実装する */
    }
}
