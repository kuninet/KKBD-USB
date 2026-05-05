#include "test_framework.h"
#include "keyrepeat.h"
#include <stdint.h>
#include <stdbool.h>

/* keyrepeat_decide の純粋関数テスト（ホスト側で実行） */

TEST(test_keyrepeat_constants) {
    EXPECT_EQ(500u, KEYREPEAT_INITIAL_DELAY_MS);
    EXPECT_EQ(50u,  KEYREPEAT_INTERVAL_MS);
}

TEST(test_keyrepeat_decide_initial_delay_noop) {
    /* 初回遅延中は FIRE しない（500ms 未満） */
    EXPECT_EQ(KEYREPEAT_NOOP, keyrepeat_decide(0,        0, false));
    EXPECT_EQ(KEYREPEAT_NOOP, keyrepeat_decide(100000,   0, false));   /* 100ms */
    EXPECT_EQ(KEYREPEAT_NOOP, keyrepeat_decide(499999,   0, false));   /* 499.999ms */
}

TEST(test_keyrepeat_decide_initial_fire) {
    /* 500ms ちょうど・以降で FIRE */
    EXPECT_EQ(KEYREPEAT_FIRE, keyrepeat_decide(500000,   0, false));   /* 500ms */
    EXPECT_EQ(KEYREPEAT_FIRE, keyrepeat_decide(1000000,  0, false));   /* 1s */
    EXPECT_EQ(KEYREPEAT_FIRE, keyrepeat_decide(10000000, 0, false));   /* 10s */
}

TEST(test_keyrepeat_decide_interval_noop) {
    /* initial_fired=true で、50ms 未満は FIRE しない */
    EXPECT_EQ(KEYREPEAT_NOOP, keyrepeat_decide(500000,   500000, true));   /* 0ms */
    EXPECT_EQ(KEYREPEAT_NOOP, keyrepeat_decide(510000,   500000, true));   /* 10ms */
    EXPECT_EQ(KEYREPEAT_NOOP, keyrepeat_decide(549999,   500000, true));   /* 49.999ms */
}

TEST(test_keyrepeat_decide_interval_fire) {
    /* initial_fired=true で、50ms 以上経過で FIRE */
    EXPECT_EQ(KEYREPEAT_FIRE, keyrepeat_decide(550000,   500000, true));   /* 50ms */
    EXPECT_EQ(KEYREPEAT_FIRE, keyrepeat_decide(600000,   500000, true));   /* 100ms */
    EXPECT_EQ(KEYREPEAT_FIRE, keyrepeat_decide(2000000,  500000, true));   /* 1.5s */
}

TEST(test_keyrepeat_decide_now_before_last_fire_safe) {
    /* now_us が last_fire_us より小さい場合（時刻巻き戻り、想定外）でも未発火扱い */
    EXPECT_EQ(KEYREPEAT_NOOP, keyrepeat_decide(0,        500000, false));
    EXPECT_EQ(KEYREPEAT_NOOP, keyrepeat_decide(100000,   500000, true));
}

int main(void) {
    RUN_TEST(test_keyrepeat_constants);
    RUN_TEST(test_keyrepeat_decide_initial_delay_noop);
    RUN_TEST(test_keyrepeat_decide_initial_fire);
    RUN_TEST(test_keyrepeat_decide_interval_noop);
    RUN_TEST(test_keyrepeat_decide_interval_fire);
    RUN_TEST(test_keyrepeat_decide_now_before_last_fire_safe);
    TEST_SUMMARY();
    return 0;
}
