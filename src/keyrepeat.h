#ifndef KEYREPEAT_H
#define KEYREPEAT_H

#include <stdint.h>
#include <stdbool.h>

/* キーリピート設定定数 */
#define KEYREPEAT_INITIAL_DELAY_MS   500u   /* キーリピート初回遅延 [ms] */
#define KEYREPEAT_INTERVAL_MS         50u   /* キーリピート間隔 [ms] */

/* 既存 API */
void keyrepeat_init(void);
void keyrepeat_register(uint8_t ascii);
void keyrepeat_cancel(void);
void keyrepeat_task(void);

/* ----- ホスト側ユニットテスト用に公開する純粋関数（GPIO/SDK非依存） ----- */

typedef enum {
    KEYREPEAT_NOOP = 0,   /* 送信しない */
    KEYREPEAT_FIRE        /* 送信する */
} keyrepeat_decision_t;

/**
 * @brief 現在時刻と最終発火時刻から、リピート発火すべきかを判定する純粋関数
 * @param now_us         現在時刻（マイクロ秒）
 * @param last_fire_us   最終発火時刻（マイクロ秒。初回は keyrepeat_register 時刻）
 * @param initial_fired  初回遅延を消化したかどうか（false: 初回遅延 500ms 待ち、true: 50ms 間隔モード）
 * @return KEYREPEAT_FIRE: 送信すべき、KEYREPEAT_NOOP: まだ送信しない
 */
keyrepeat_decision_t keyrepeat_decide(uint64_t now_us,
                                      uint64_t last_fire_us,
                                      bool initial_fired);

#endif /* KEYREPEAT_H */
