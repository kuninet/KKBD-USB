#include "led.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

/* Phase 1: 起動時BOOT状態で500ms周期の単純点滅を実装。
 * Phase 6 で WAIT_DEVICE / MOUNTED / TX / ERROR 各状態の点滅パターンを追加する。
 *
 * Phase 1 暫定: BOOT状態を点滅で動作確認（Phase 6で常時点灯に置換）
 *
 * 注: 設計書（FR-010）の LED_STATE_BOOT の最終仕様は「常時点灯」だが、
 * Phase 1 では Lチカで Pico SDK ビルド・書き込みパスの動作確認を兼ねるため、
 * 暫定的に 500ms 周期の点滅としている。Phase 6 で本来仕様（常時点灯）に置換予定。
 */

/* 内部状態の方式: 設計書 §3.7.4 では last_toggle_us 方式を採用しているが、
 * 本実装は Pico SDK 標準の absolute_time_t 型による next_toggle 方式に変更。
 * SDK 推奨パターン（time_reached/make_timeout_time_ms）に整合。
 */

#define LED_GPIO              25u
#define LED_BOOT_BLINK_MS     500u  /* Phase 1: BOOT状態の点滅周期（トグル間隔） */
/* Phase 6 で以下の定数を追加予定:
 *   LED_BLINK_FAST_MS  100u  -- ERROR 状態用（高速点滅）
 *   LED_BLINK_SLOW_MS  500u  -- WAIT_DEVICE 状態用（低速点滅）
 */

static led_state_t s_current_state = LED_STATE_BOOT;
static absolute_time_t s_next_toggle;
static bool s_led_on = false;

void led_init(void) {
    gpio_init(LED_GPIO);
    gpio_set_dir(LED_GPIO, GPIO_OUT);
    gpio_put(LED_GPIO, 0);
    s_led_on = false;
    s_next_toggle = make_timeout_time_ms(LED_BOOT_BLINK_MS);
}

void led_set_state(led_state_t state) {
    /* TODO: Phase 6 で状態に応じた挙動を切替（点灯/低速点滅/短時間点滅/高速点滅） */
    s_current_state = state;
}

void led_task(void) {
    /* Phase 1: BOOT状態のまま500ms周期で単純点滅。
     * Phase 6 で s_current_state に応じた点滅パターンに置換予定。
     */
    if (time_reached(s_next_toggle)) {
        s_led_on = !s_led_on;
        gpio_put(LED_GPIO, s_led_on ? 1 : 0);
        s_next_toggle = make_timeout_time_ms(LED_BOOT_BLINK_MS);
    }
    (void)s_current_state; /* Phase 1 では未参照、Phase 6 で参照 */
}
