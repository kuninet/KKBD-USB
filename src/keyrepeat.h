#ifndef KEYREPEAT_H
#define KEYREPEAT_H

#include <stdint.h>

/* キーリピート設定定数 */
#define KEYREPEAT_INITIAL_DELAY_MS   500u   /* キーリピート初回遅延 [ms] */
#define KEYREPEAT_INTERVAL_MS         50u   /* キーリピート間隔 [ms] */

/**
 * @brief キーリピート状態を初期化する
 */
void keyrepeat_init(void);

/**
 * @brief キーダウン時に呼び出す。リピート対象キーを登録する。
 * @param ascii  リピート送信するASCIIコード
 */
void keyrepeat_register(uint8_t ascii);

/**
 * @brief キーアップ時（全キーリリース）に呼び出す。リピートをキャンセルする。
 */
void keyrepeat_cancel(void);

/**
 * @brief メインループから定期的に呼び出す。タイマー判定とUART送信を行う。
 */
void keyrepeat_task(void);

#endif /* KEYREPEAT_H */
