#include "uart_out.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

/* Phase 2 実装:
 * - UART0 を初期化（GPIO 0 = TX）
 * - ボーレートは config_get_baudrate() の値を使用（init 引数で受ける）
 * - 8N1、フロー制御なし、FIFO 有効、ブロッキング送信
 */

void uart_out_init(uint32_t baudrate) {
    uart_init(UART_ID, baudrate);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, UART_DATA_BITS, UART_STOP_BITS, UART_PARITY);
    uart_set_fifo_enabled(UART_ID, true);
    /* 受信は本プロジェクトでは未使用。送信のみブロッキングで運用 */
    uart_set_hw_flow(UART_ID, false, false);
}

void uart_out_putc(uint8_t byte) {
    /* SDK の uart_putc() は PICO_UART_ENABLE_CRLF_SUPPORT 有効時に '\n' を '\r\n' に
     * 自動変換する。これは FR-004（ジャンパーで CR/LF/CRLF を選択）の意図を破壊するため、
     * 変換しない uart_putc_raw() を使用する。
     */
    uart_putc_raw(UART_ID, (char)byte);
}

void uart_out_puts(const char *s) {
    if (s == NULL) {
        return;
    }
    while (*s) {
        uart_out_putc((uint8_t)*s);
        s++;
    }
}

void uart_out_send_line_ending(void) {
    line_ending_t le = config_get_line_ending();
    switch (le) {
        case LINE_ENDING_CR:
            uart_out_putc(0x0D);
            break;
        case LINE_ENDING_LF:
            uart_out_putc(0x0A);
            break;
        case LINE_ENDING_CRLF:
            uart_out_putc(0x0D);
            uart_out_putc(0x0A);
            break;
        default:
            /* 予約パターンは config 側で CR にフォールバック済み */
            uart_out_putc(0x0D);
            break;
    }
}
