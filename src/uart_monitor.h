#ifndef UART_MONITOR_H
#define UART_MONITOR_H

#include <stdint.h>
#include "hardware/uart.h"  /* uart1 / UART_PARITY_NONE 等 SDK マクロ参照のため */

/* UART モニタ設定定数（UART1 を使用） */
#define UART_MON_ID         uart1
#define UART_MON_TX_PIN     4    /* UART1 TX = GPIO 4 = Pico Pin 6 */
#define UART_MON_RX_PIN     1    /* UART0 RX = GPIO 1 = Pico Pin 2（SBC TX 受信、UART0 RX を流用しモニタへパススルー） */
#define UART_MON_DATA_BITS  8
#define UART_MON_STOP_BITS  1
#define UART_MON_PARITY     UART_PARITY_NONE

/**
 * @brief UART モニタ初期化
 *
 * UART1 を TX 専用で初期化、UART0 RX 側 GPIO 機能を割当。
 * @param baudrate ボーレート（UART0 と同じ値）
 */
void uart_monitor_init(uint32_t baudrate);

/**
 * @brief メインループから定期呼出。UART0 RX を読み UART1 TX へパススルー
 */
void uart_monitor_task(void);

/**
 * @brief モニタ側に 1 バイト送信（ログ / ローカルエコー用）
 */
void uart_monitor_putc(uint8_t byte);

/**
 * @brief モニタ側に NUL 終端文字列送信
 */
void uart_monitor_puts(const char *s);

/**
 * @brief 行末コードをモニタ側に送信する（ローカルエコー用）
 *        config_get_line_ending() の設定に従って CR/LF/CRLF を送る
 */
void uart_monitor_send_line_ending(void);

#endif /* UART_MONITOR_H */
