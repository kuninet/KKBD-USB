#include "config.h"

/* TODO: implement in Phase 2 */
/* Read jumper pin settings for UART baud rate (2-bit) and line ending (2-bit) */
/* Baud rate jumper: 00=9600, 01=19200, 10=38400, 11=115200 */
/* Line ending jumper: 00=CR, 01=LF, 10=CRLF, 11=予約(CR) */

void config_init(void) {
    /* TODO: implement in Phase 2 - initialize GPIO pins for jumper reading */
}

uint32_t config_get_baudrate(void) {
    /* TODO: implement in Phase 2 - read baud rate jumper and return corresponding value */
    return 9600;
}

line_ending_t config_get_line_ending(void) {
    /* TODO: implement in Phase 2 - read line ending jumper and return corresponding enum */
    /* 予約パターン(JP1=SHORT, JP2=SHORT)のデフォルト動作と整合させ LINE_ENDING_CR を返す */
    return LINE_ENDING_CR;
}
