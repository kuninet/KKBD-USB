#ifndef UART_OUT_H
#define UART_OUT_H

#include <stdint.h>

/* UART設定定数 */
#define UART_ID         uart0
#define UART_TX_PIN     0       /* GPIO 0 を TX として使用 */
#define UART_DATA_BITS  8
#define UART_STOP_BITS  1
#define UART_PARITY     UART_PARITY_NONE

/**
 * @brief UART初期化
 * @param baudrate  ボーレート（9600, 19200, 38400, 115200）
 */
void uart_out_init(uint32_t baudrate);

/**
 * @brief 1バイト送信
 * @param byte  送信バイト
 */
void uart_out_putc(uint8_t byte);

/**
 * @brief 文字列送信
 * @param s  送信文字列（NUL終端）
 */
void uart_out_puts(const char *s);

/**
 * @brief 行末コードを送信する（Enterキー入力時に呼ぶ）
 *        送信内容は config_get_line_ending() の設定による
 */
void uart_out_send_line_ending(void);

#endif /* UART_OUT_H */
