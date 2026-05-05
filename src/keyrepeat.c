#include "keyrepeat.h"

#ifndef KKBD_HOST_TEST
#include "pico/stdlib.h"
#include "pico/time.h"
#include "uart_out.h"
#include "led.h"
#endif

/* Phase 6 実装:
 *   - keyrepeat_register: キーダウン時に呼ぶ。リピート対象を登録
 *   - keyrepeat_cancel:   キーアップ（全キー離した）時に呼ぶ
 *   - keyrepeat_task:     メインループから定期呼出。500ms初回遅延後、50ms間隔で発火
 *   - keyrepeat_decide:   時刻判定の純粋関数（ホストテスト可能）
 *
 * Pico SDK 依存箇所（time API, uart_out, led）は KKBD_HOST_TEST マクロで除外し、
 * ホスト側でも keyrepeat_decide 等の純粋関数のテスト可能とする。
 */

/* ----- 純粋関数（ホスト/Pico 共通） ----- */

keyrepeat_decision_t keyrepeat_decide(uint64_t now_us,
                                      uint64_t last_fire_us,
                                      bool initial_fired) {
    uint64_t const elapsed_us = (now_us >= last_fire_us)
                                ? (now_us - last_fire_us) : 0;
    uint64_t const threshold_us = initial_fired
        ? ((uint64_t)KEYREPEAT_INTERVAL_MS * 1000ULL)
        : ((uint64_t)KEYREPEAT_INITIAL_DELAY_MS * 1000ULL);

    return (elapsed_us >= threshold_us) ? KEYREPEAT_FIRE : KEYREPEAT_NOOP;
}

/* ----- 内部状態（Pico SDK ビルド時のみ使用） ----- */

#ifndef KKBD_HOST_TEST

static bool     s_active        = false;
static uint8_t  s_ascii         = 0;
static uint64_t s_last_fire_us  = 0;
static bool     s_initial_fired = false;

static uint64_t now_us_(void) {
    return to_us_since_boot(get_absolute_time());
}

void keyrepeat_init(void) {
    s_active        = false;
    s_ascii         = 0;
    s_last_fire_us  = 0;
    s_initial_fired = false;
}

void keyrepeat_register(uint8_t ascii) {
    s_active        = true;
    s_ascii         = ascii;
    s_last_fire_us  = now_us_();
    s_initial_fired = false;
}

void keyrepeat_cancel(void) {
    s_active        = false;
    s_ascii         = 0;
    s_initial_fired = false;
}

void keyrepeat_task(void) {
    if (!s_active) {
        return;
    }
    uint64_t const now = now_us_();
    if (keyrepeat_decide(now, s_last_fire_us, s_initial_fired) == KEYREPEAT_FIRE) {
        uart_out_putc(s_ascii);
        led_set_state(LED_STATE_TX);
        s_last_fire_us  = now;
        s_initial_fired = true;
    }
}

#else /* KKBD_HOST_TEST: ホスト側ビルドではダミー実装 */

/* ホスト側テストでは keyrepeat_decide のみテスト対象とし、
 * register/cancel/task/init はリンク用のダミー実装として提供。 */

void keyrepeat_init(void)            { /* no-op */ }
void keyrepeat_register(uint8_t a)   { (void)a; }
void keyrepeat_cancel(void)          { /* no-op */ }
void keyrepeat_task(void)            { /* no-op */ }

#endif /* KKBD_HOST_TEST */
