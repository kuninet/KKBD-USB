#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

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

/* JP5: ローカルエコー（モニタ側） bit */
#define JP5_GPIO    14   /* OPEN=エコー OFF / SHORT=エコー ON */

void config_init(void);
uint32_t config_get_baudrate(void);
line_ending_t config_get_line_ending(void);

/**
 * @brief JP5 の状態に応じてローカルエコー有効/無効を返す
 * @return true: エコー有効 (JP5=SHORT) / false: 無効 (JP5=OPEN)
 */
bool config_get_local_echo(void);

/* ----- ホスト側ユニットテスト用に公開する純粋関数（GPIO非依存） ----- */

/**
 * @brief 2ビット値からボーレートを解決する純粋関数
 * @param bits 下位2ビット (bit0=JP3, bit1=JP4) ※ SHORT=1 の論理ビット
 * @return ボーレート（9600, 19200, 38400, 115200）
 */
uint32_t config_decode_baudrate(uint8_t bits);

/**
 * @brief 2ビット値から行末コードを解決する純粋関数
 * @param bits 下位2ビット (bit0=JP1, bit1=JP2) ※ SHORT=1 の論理ビット
 * @return line_ending_t（予約パターン 0b11 は LINE_ENDING_CR にフォールバック）
 */
line_ending_t config_decode_line_ending(uint8_t bits);

/**
 * @brief 1 ビットからローカルエコー有無を解決する純粋関数（テスト用）
 * @param bit JP5 の論理ビット（SHORT=1）
 * @return true: エコー ON / false: OFF
 */
bool config_decode_local_echo(uint8_t bit);

#endif /* CONFIG_H */
