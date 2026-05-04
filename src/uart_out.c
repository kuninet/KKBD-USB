#include "uart_out.h"
#include "config.h"

/* TODO: implement in Phase 2 */
/* UART output module for sending ASCII characters to SBC6800/KZ80 */

void uart_out_init(uint32_t baudrate) {
    /* TODO: implement in Phase 2 - initialize UART peripheral with given baud rate */
    (void)baudrate;
}

void uart_out_putc(uint8_t byte) {
    /* TODO: implement in Phase 2 - transmit single byte over UART */
    (void)byte;
}

void uart_out_puts(const char *s) {
    /* TODO: implement in Phase 2 - transmit string over UART */
    (void)s;
}

void uart_out_send_line_ending(void) {
    /* TODO: implement in Phase 2 - send CR, LF, or CRLF based on config_get_line_ending() */
}
