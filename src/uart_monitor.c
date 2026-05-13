#include "uart_monitor.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "config.h"

/* Phase 9 実装: uart_monitor — SBC 応答パススルー + ログ/エコー出力
 *
 * 目的:
 *   - UART0 RX (GPIO 1) で SBC からの応答を受信し、UART1 TX (GPIO 4) に素通しする。
 *     PC 上の USB-Serial 変換器（FT232RL / CP2102 / CH340 等）経由でターミナルに表示。
 *   - ログ送信 API も兼ねる。main.c / usb_host.c の起動バナーや [USB] ログを
 *     uart_monitor_puts() 経由で UART1 に流す用途にも使用できる。
 *
 * 方針:
 *   - 満杯時ドロップのベストエフォート方針を採る。
 *     FIFO ブロッキングを回避し、USB ホスト処理の遅延を防ぐ。
 *   - SBC からの受信データは無加工で素通しする（uart_putc_raw 使用）。
 *     CR/LF/エスケープシーケンスをそのまま PC に届ける。
 */

void uart_monitor_init(uint32_t baudrate) {
    /* UART1 を指定ボーレートで初期化（TX 専用） */
    uart_init(UART_MON_ID, baudrate);

    /* GPIO 4 を UART1 TX に割当 */
    gpio_set_function(UART_MON_TX_PIN, GPIO_FUNC_UART);

    /* GPIO 1 を UART0 RX に割当（uart0 は uart_out_init() で既に初期化済み） */
    gpio_set_function(UART_MON_RX_PIN, GPIO_FUNC_UART);

    /* UART1 通信フォーマット設定: 8N1 */
    uart_set_format(UART_MON_ID, UART_MON_DATA_BITS, UART_MON_STOP_BITS, UART_MON_PARITY);

    /* FIFO 有効化（TX バッファを最大限活用） */
    uart_set_fifo_enabled(UART_MON_ID, true);

    /* ハードウェアフロー制御なし（TX 専用のため不要） */
    uart_set_hw_flow(UART_MON_ID, false, false);
}

void uart_monitor_task(void) {
    /* UART0 RX に届いたデータを UART1 TX へ素通し。
     * UART1 TX FIFO が満杯なら当該バイトをドロップ（モニタ用途なので許容）。 */
    while (uart_is_readable(uart0)) {
        uint8_t b = (uint8_t)uart_getc(uart0);
        if (uart_is_writable(uart1)) {
            uart_putc_raw(uart1, (char)b);
        }
        /* 満杯時はドロップ */
    }
}

void uart_monitor_putc(uint8_t byte) {
    /* ベストエフォート: FIFO に空きがあれば送信、満杯ならドロップ */
    if (uart_is_writable(uart1)) {
        uart_putc_raw(uart1, (char)byte);
    }
}

void uart_monitor_puts(const char *s) {
    /* NULL ガード */
    if (s == NULL) {
        return;
    }
    /* 1 バイトずつ uart_monitor_putc() に流す */
    while (*s) {
        uart_monitor_putc((uint8_t)*s);
        s++;
    }
}

void uart_monitor_send_line_ending(void) {
    line_ending_t le = config_get_line_ending();
    switch (le) {
        case LINE_ENDING_CR:   uart_monitor_putc(0x0D); break;
        case LINE_ENDING_LF:   uart_monitor_putc(0x0A); break;
        case LINE_ENDING_CRLF: uart_monitor_putc(0x0D); uart_monitor_putc(0x0A); break;
        default:               uart_monitor_putc(0x0D); break;  /* 予約パターンは config 側で CR にフォールバック済み */
    }
}
