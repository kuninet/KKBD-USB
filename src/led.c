#include "led.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

/* Phase 6 実装: LED 状態機械（設計書 §3.7、§9.1）
 *
 * 状態別挙動:
 *   LED_STATE_BOOT          : 常時点灯（起動表示）
 *   LED_STATE_WAIT_DEVICE   : 低速点滅 500ms トグル（USB キーボード待機）
 *   LED_STATE_MOUNTED       : 常時点灯（USB キーボード接続済み）
 *   LED_STATE_TX            : 30ms 短時間点灯（送信時）。期限到達で前状態に自動復帰
 *   LED_STATE_ERROR         : 高速点滅 100ms トグル（エラー表示）
 *
 * TX 状態の戻り先:
 *   led_set_state(LED_STATE_TX) 時に s_prev_state へ現在状態を保存。
 *   TX 期限到達時に s_current_state = s_prev_state で復帰。
 *   既に TX 中の再呼出は s_prev_state 非更新（連続送信を MOUNTED 経由でなく
 *   TX 連続として扱う）。
 */

#define LED_GPIO              25u
#define LED_BLINK_FAST_MS    100u  /* ERROR */
#define LED_BLINK_SLOW_MS    500u  /* WAIT_DEVICE */
#define LED_TX_PULSE_MS       30u  /* TX 点灯時間 */

static led_state_t     s_current_state    = LED_STATE_BOOT;
static led_state_t     s_prev_state       = LED_STATE_BOOT;
static absolute_time_t s_next_toggle;
static absolute_time_t s_tx_end;
static bool            s_led_on           = false;
/* TX 復帰時の位相復元用 */
static bool            s_prev_led_on      = false;
static absolute_time_t s_prev_next_toggle;

static void apply_state_initial_(led_state_t state) {
    /* 状態切替直後の LED 出力と次回トグル時刻を設定 */
    switch (state) {
        case LED_STATE_BOOT:
        case LED_STATE_MOUNTED:
            s_led_on = true;
            gpio_put(LED_GPIO, 1);
            break;
        case LED_STATE_WAIT_DEVICE:
            s_led_on = true;
            gpio_put(LED_GPIO, 1);
            s_next_toggle = make_timeout_time_ms(LED_BLINK_SLOW_MS);
            break;
        case LED_STATE_ERROR:
            s_led_on = true;
            gpio_put(LED_GPIO, 1);
            s_next_toggle = make_timeout_time_ms(LED_BLINK_FAST_MS);
            break;
        case LED_STATE_TX:
            s_led_on = true;
            gpio_put(LED_GPIO, 1);
            s_tx_end = make_timeout_time_ms(LED_TX_PULSE_MS);
            break;
    }
}

void led_init(void) {
    gpio_init(LED_GPIO);
    gpio_set_dir(LED_GPIO, GPIO_OUT);
    s_current_state = LED_STATE_BOOT;
    s_prev_state    = LED_STATE_BOOT;
    apply_state_initial_(LED_STATE_BOOT);
}

void led_set_state(led_state_t state) {
    if (state == LED_STATE_TX) {
        if (s_current_state != LED_STATE_TX) {
            /* TX 進入: 復帰用に現在の位相情報を保存 */
            s_prev_state       = s_current_state;
            s_prev_led_on      = s_led_on;
            s_prev_next_toggle = s_next_toggle;
        }
        s_current_state = LED_STATE_TX;
        /* TX 状態の出力（LED ON、s_tx_end 設定） */
        s_led_on = true;
        gpio_put(LED_GPIO, 1);
        s_tx_end = make_timeout_time_ms(LED_TX_PULSE_MS);
        return;
    }
    /* 通常の状態遷移 */
    s_current_state = state;
    s_prev_state    = state;
    apply_state_initial_(state);
}

void led_task(void) {
    switch (s_current_state) {
        case LED_STATE_BOOT:
        case LED_STATE_MOUNTED:
            /* 常時点灯: 何もしない */
            break;
        case LED_STATE_WAIT_DEVICE:
            if (time_reached(s_next_toggle)) {
                s_led_on = !s_led_on;
                gpio_put(LED_GPIO, s_led_on ? 1 : 0);
                s_next_toggle = make_timeout_time_ms(LED_BLINK_SLOW_MS);
            }
            break;
        case LED_STATE_ERROR:
            if (time_reached(s_next_toggle)) {
                s_led_on = !s_led_on;
                gpio_put(LED_GPIO, s_led_on ? 1 : 0);
                s_next_toggle = make_timeout_time_ms(LED_BLINK_FAST_MS);
            }
            break;
        case LED_STATE_TX:
            if (time_reached(s_tx_end)) {
                /* TX 期限到達: 前状態へ復帰し、保存しておいた位相を復元 */
                s_current_state = s_prev_state;
                if (s_prev_state == LED_STATE_BOOT || s_prev_state == LED_STATE_MOUNTED) {
                    /* 常時点灯 */
                    s_led_on = true;
                    gpio_put(LED_GPIO, 1);
                } else {
                    /* WAIT_DEVICE / ERROR: 点滅位相を復元 */
                    s_led_on = s_prev_led_on;
                    gpio_put(LED_GPIO, s_led_on ? 1 : 0);
                    s_next_toggle = s_prev_next_toggle;
                }
            }
            break;
    }
}
