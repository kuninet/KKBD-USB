#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

typedef enum {
    LINE_ENDING_CR   = 0,  /* JP1=OPEN, JP2=OPEN  → CR (0x0D) */
    LINE_ENDING_LF   = 1,  /* JP1=SHORT, JP2=OPEN → LF (0x0A) */
    LINE_ENDING_CRLF = 2,  /* JP1=OPEN, JP2=SHORT → CRLF (0x0D 0x0A) */
    /* JP1=SHORT, JP2=SHORT は予約。LINE_ENDING_CR として動作 */
} line_ending_t;

/* GPIO ピン定数 */
#define JP1_GPIO    10   /* 行末コード bit0 */
#define JP2_GPIO    11   /* 行末コード bit1 */
#define JP3_GPIO    12   /* ボーレート bit0 */
#define JP4_GPIO    13   /* ボーレート bit1 */

void config_init(void);
uint32_t config_get_baudrate(void);
line_ending_t config_get_line_ending(void);

#endif /* CONFIG_H */
