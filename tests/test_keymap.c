#include "test_framework.h"
#include "keymap.h"
#include <stdint.h>
#include <stdbool.h>

/* ========================================
 * 現時点で通るテスト（定数検証）
 * ======================================== */

TEST(test_modifier_constants) {
    EXPECT_EQ(0x01, MOD_LCTRL);
    EXPECT_EQ(0x02, MOD_LSHIFT);
    EXPECT_EQ(0x04, MOD_LALT);
    EXPECT_EQ(0x08, MOD_LGUI);
    EXPECT_EQ(0x10, MOD_RCTRL);
    EXPECT_EQ(0x20, MOD_RSHIFT);
    EXPECT_EQ(0x40, MOD_RALT);
    EXPECT_EQ(0x80, MOD_RGUI);
    EXPECT_EQ(MOD_LSHIFT | MOD_RSHIFT, MOD_SHIFT);
    EXPECT_EQ(MOD_LCTRL  | MOD_RCTRL,  MOD_CTRL);
}

/* ========================================
 * Phase 4 実装後に有効化するテスト
 * keymap.c の英数字 + Enter 基本実装完了後に
 * KKBD_PHASE4_DONE を定義して有効化する
 * ======================================== */

TEST(test_keymap_convert_lowercase) {
    /* HID Usage ID 0x04 (a) → 'a' (0x61) */
    EXPECT_EQ('a', keymap_convert(0x04, 0));
    EXPECT_EQ('b', keymap_convert(0x05, 0));
    EXPECT_EQ('z', keymap_convert(0x1D, 0));
}

TEST(test_keymap_convert_digits) {
    /* HID Usage ID 0x1E (1) → '1' (0x31) */
    EXPECT_EQ('1', keymap_convert(0x1E, 0));
    EXPECT_EQ('0', keymap_convert(0x27, 0));
}

TEST(test_keymap_convert_unsupported) {
    /* 範囲下限の直前（Reserved/Error コード） */
    EXPECT_EQ(0x00, keymap_convert(0x00, 0));     /* Reserved */
    EXPECT_EQ(0x00, keymap_convert(0x01, 0));     /* ErrorRollOver */
    EXPECT_EQ(0x00, keymap_convert(0x03, 0));     /* ErrorUndefined */
    /* Enter は keymap_is_enter で判定するため convert は 0 を返す */
    EXPECT_EQ(0x00, keymap_convert(0x28, 0));     /* Enter（変換は0、is_enter で判定） */
    EXPECT_EQ(0x00, keymap_convert(0x58, 0));     /* Keypad Enter */
    /* Phase 5 で実装予定の特殊・記号キー（Phase 4 では未対応） */
    EXPECT_EQ(0x00, keymap_convert(0x29, 0));     /* Esc（Phase 5 対応予定） */
    EXPECT_EQ(0x00, keymap_convert(0x2A, 0));     /* Backspace（Phase 5 対応予定） */
    EXPECT_EQ(0x00, keymap_convert(0x2B, 0));     /* Tab（Phase 5 対応予定） */
    EXPECT_EQ(0x00, keymap_convert(0x2C, 0));     /* Space（Phase 5 対応予定） */
    /* F キー */
    EXPECT_EQ(0x00, keymap_convert(0x3A, 0));     /* F1 */
    /* 範囲外（最上位） */
    EXPECT_EQ(0x00, keymap_convert(0xFF, 0));     /* 範囲外 */
}

TEST(test_keymap_is_enter) {
    EXPECT_TRUE(keymap_is_enter(0x28));   /* Return Enter */
    EXPECT_TRUE(keymap_is_enter(0x58));   /* Keypad Enter */
    EXPECT_FALSE(keymap_is_enter(0x04));  /* 'a' は Enter ではない */
    EXPECT_FALSE(keymap_is_enter(0x29));  /* Esc は Enter ではない */
}

/* ========================================
 * Phase 5 実装後に有効化するテスト
 * Shift/Ctrl 修飾・特殊キー実装完了後に
 * KKBD_PHASE5_DONE を定義して有効化する
 * ======================================== */

TEST(test_keymap_convert_shift_uppercase) {
    /* Shift+a → 'A' */
    EXPECT_EQ('A', keymap_convert(0x04, MOD_LSHIFT));
    EXPECT_EQ('A', keymap_convert(0x04, MOD_RSHIFT));
    EXPECT_EQ('Z', keymap_convert(0x1D, MOD_LSHIFT));
}

TEST(test_keymap_convert_ctrl_control_chars) {
    /* Ctrl+a → 0x01 (SOH) */
    EXPECT_EQ(0x01, keymap_convert(0x04, MOD_LCTRL));
    EXPECT_EQ(0x03, keymap_convert(0x06, MOD_LCTRL)); /* Ctrl+c → ETX */
    EXPECT_EQ(0x1A, keymap_convert(0x1D, MOD_LCTRL)); /* Ctrl+z → SUB */
}

TEST(test_keymap_convert_special_keys) {
    EXPECT_EQ(0x1B, keymap_convert(0x29, 0)); /* Esc */
    EXPECT_EQ(0x09, keymap_convert(0x2B, 0)); /* Tab */
    EXPECT_EQ(0x20, keymap_convert(0x2C, 0)); /* Space */
    EXPECT_EQ(0x08, keymap_convert(0x2A, 0)); /* Backspace → BS */
}

int main(void) {
    /* 現時点で通るテスト */
    RUN_TEST(test_modifier_constants);

#ifdef KKBD_PHASE4_DONE
    /* Phase 4 (keymap英数字+Enter+未対応キー判定) 実装後に有効化 */
    RUN_TEST(test_keymap_convert_lowercase);
    RUN_TEST(test_keymap_convert_digits);
    RUN_TEST(test_keymap_convert_unsupported);
    RUN_TEST(test_keymap_is_enter);
#endif

#ifdef KKBD_PHASE5_DONE
    /* Phase 5 (Shift/Ctrl/特殊キー) 実装後に有効化 */
    RUN_TEST(test_keymap_convert_shift_uppercase);
    RUN_TEST(test_keymap_convert_ctrl_control_chars);
    RUN_TEST(test_keymap_convert_special_keys);
#endif

    TEST_SUMMARY();
    return 0;
}
