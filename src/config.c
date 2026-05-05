#include "config.h"
#include <stddef.h>

#ifndef KKBD_HOST_TEST
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#endif

/* Phase 2 実装:
 * - 起動時に JP1〜JP4 をプルアップ入力で初期化
 * - GPIO 値を反転して論理ビット（SHORT=1）化
 * - decode 関数で値に変換し、内部状態に保存
 * - getter は内部状態を返すだけ
 *
 * decode 関数はホスト側ユニットテストで検証可能（GPIO 非依存）。
 * KKBD_HOST_TEST=1 でビルドすると Pico SDK 依存コードが除外される。
 */

uint32_t config_decode_baudrate(uint8_t bits) {
    switch (bits & 0x03) {
        case 0x00: return 9600;
        case 0x01: return 19200;
        case 0x02: return 38400;
        case 0x03: return 115200;
        default:   return 9600; /* unreachable */
    }
}

line_ending_t config_decode_line_ending(uint8_t bits) {
    switch (bits & 0x03) {
        case 0x00: return LINE_ENDING_CR;
        case 0x01: return LINE_ENDING_LF;
        case 0x02: return LINE_ENDING_CRLF;
        case 0x03: return LINE_ENDING_CR; /* 予約パターンは CR にフォールバック */
        default:   return LINE_ENDING_CR; /* unreachable */
    }
}

#ifndef KKBD_HOST_TEST

/* GPIO直接操作する関数とstatic変数はSDKビルド時のみ */

static uint32_t      s_baudrate    = 9600;
static line_ending_t s_line_ending = LINE_ENDING_CR;

/* GPIO 値（プルアップで OPEN=1, SHORT=0）を読み、論理ビット（SHORT=1）に反転 */
static uint8_t read_jumper_bits(uint pin_low, uint pin_high) {
    uint8_t bit0 = gpio_get(pin_low)  ? 0U : 1U;  /* SHORT 時に 1 */
    uint8_t bit1 = gpio_get(pin_high) ? 0U : 1U;
    return (uint8_t)((bit1 << 1) | bit0);
}

void config_init(void) {
    const uint pins[] = { JP1_GPIO, JP2_GPIO, JP3_GPIO, JP4_GPIO };
    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }

    /* プルアップ後、入力値が安定するまで少し待つ */
    sleep_us(10);

    uint8_t br_bits = read_jumper_bits(JP3_GPIO, JP4_GPIO);
    uint8_t le_bits = read_jumper_bits(JP1_GPIO, JP2_GPIO);

    s_baudrate    = config_decode_baudrate(br_bits);
    s_line_ending = config_decode_line_ending(le_bits);
}

uint32_t config_get_baudrate(void) {
    return s_baudrate;
}

line_ending_t config_get_line_ending(void) {
    return s_line_ending;
}

#endif /* KKBD_HOST_TEST */
