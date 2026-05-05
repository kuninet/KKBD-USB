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
    /* 注: Esc/BS/Tab/Space は Phase 5 で実装済（s_normal_table に登録）。
     * Phase 4 only ビルド（KKBD_PHASE5_DONE=OFF）では keymap.c の現実装と
     * 矛盾するため、以下のチェックは Phase 5 OFF 時のみ意味を持つ（履歴的役割）。
     * 推奨: Phase 5 完了後は -DKKBD_PHASE5_DONE=ON も指定すること。
     */
#ifndef KKBD_PHASE5_DONE
    /* Phase 5 で実装予定の特殊・記号キー（Phase 4 単独ビルドでのみチェック） */
    EXPECT_EQ(0x00, keymap_convert(0x29, 0));     /* Esc */
    EXPECT_EQ(0x00, keymap_convert(0x2A, 0));     /* Backspace */
    EXPECT_EQ(0x00, keymap_convert(0x2B, 0));     /* Tab */
    EXPECT_EQ(0x00, keymap_convert(0x2C, 0));     /* Space */
#endif
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
 * Shift/Ctrl 修飾・特殊キー・記号キー・テンキー実装完了後に
 * KKBD_PHASE5_DONE を定義して有効化する
 *
 * テスト一覧:
 *   test_keymap_convert_shift_uppercase    - 英字 Shift → 大文字（設計書 §5.3）
 *   test_keymap_convert_shift_symbols      - 数字 Shift → 記号（! @ # $ % ^ & * ( )）
 *   test_keymap_convert_symbols_no_shift   - 記号キー Shift なし（- = [ ] \ ; ' ` , . /）
 *   test_keymap_convert_symbols_with_shift - 記号キー Shift あり（_ + { } | : " ~ < > ?）
 *   test_keymap_convert_ctrl_control_chars - Ctrl+英字→制御文字、Ctrl+記号→制御文字
 *   test_keymap_convert_special_keys       - Esc/Tab/Space/Backspace
 *   test_keymap_convert_keypad             - テンキー数字・小数点（設計書 §5.2）
 *   test_keymap_convert_priority_ctrl_over_shift - Ctrl+Shift 同時押しで Ctrl 優先
 * ======================================== */

TEST(test_keymap_convert_shift_uppercase) {
    /* Shift+a → 'A'（設計書 §5.3: a〜z → A〜Z） */
    EXPECT_EQ('A', keymap_convert(0x04, MOD_LSHIFT));
    EXPECT_EQ('A', keymap_convert(0x04, MOD_RSHIFT));
    EXPECT_EQ('M', keymap_convert(0x10, MOD_LSHIFT));  /* m -> M */
    EXPECT_EQ('Z', keymap_convert(0x1D, MOD_LSHIFT));
    EXPECT_EQ('Z', keymap_convert(0x1D, MOD_RSHIFT));
}

TEST(test_keymap_convert_shift_symbols) {
    /* Shift+数字 → 記号（設計書 §5.3） */
    EXPECT_EQ('!', keymap_convert(0x1E, MOD_LSHIFT));  /* Shift+1 */
    EXPECT_EQ('@', keymap_convert(0x1F, MOD_LSHIFT));  /* Shift+2 */
    EXPECT_EQ('#', keymap_convert(0x20, MOD_LSHIFT));  /* Shift+3 */
    EXPECT_EQ('$', keymap_convert(0x21, MOD_LSHIFT));  /* Shift+4 */
    EXPECT_EQ('%', keymap_convert(0x22, MOD_LSHIFT));  /* Shift+5 */
    EXPECT_EQ('^', keymap_convert(0x23, MOD_LSHIFT));  /* Shift+6 */
    EXPECT_EQ('&', keymap_convert(0x24, MOD_LSHIFT));  /* Shift+7 */
    EXPECT_EQ('*', keymap_convert(0x25, MOD_LSHIFT));  /* Shift+8 */
    EXPECT_EQ('(', keymap_convert(0x26, MOD_LSHIFT));  /* Shift+9 */
    EXPECT_EQ(')', keymap_convert(0x27, MOD_LSHIFT));  /* Shift+0 */
}

TEST(test_keymap_convert_symbols_no_shift) {
    /* 記号キー Shift なし（設計書 §5.2） */
    EXPECT_EQ('-',  keymap_convert(0x2D, 0));
    EXPECT_EQ('=',  keymap_convert(0x2E, 0));
    EXPECT_EQ('[',  keymap_convert(0x2F, 0));
    EXPECT_EQ(']',  keymap_convert(0x30, 0));
    EXPECT_EQ('\\', keymap_convert(0x31, 0));
    EXPECT_EQ(';',  keymap_convert(0x33, 0));
    EXPECT_EQ('\'', keymap_convert(0x34, 0));
    EXPECT_EQ('`',  keymap_convert(0x35, 0));
    EXPECT_EQ(',',  keymap_convert(0x36, 0));
    EXPECT_EQ('.',  keymap_convert(0x37, 0));
    EXPECT_EQ('/',  keymap_convert(0x38, 0));
}

TEST(test_keymap_convert_symbols_with_shift) {
    /* 記号キー Shift あり（設計書 §5.3） */
    EXPECT_EQ('_',  keymap_convert(0x2D, MOD_LSHIFT));
    EXPECT_EQ('+',  keymap_convert(0x2E, MOD_LSHIFT));
    EXPECT_EQ('{',  keymap_convert(0x2F, MOD_LSHIFT));
    EXPECT_EQ('}',  keymap_convert(0x30, MOD_LSHIFT));
    EXPECT_EQ('|',  keymap_convert(0x31, MOD_LSHIFT));
    EXPECT_EQ(':',  keymap_convert(0x33, MOD_LSHIFT));
    EXPECT_EQ('"',  keymap_convert(0x34, MOD_LSHIFT));
    EXPECT_EQ('~',  keymap_convert(0x35, MOD_LSHIFT));
    EXPECT_EQ('<',  keymap_convert(0x36, MOD_LSHIFT));
    EXPECT_EQ('>',  keymap_convert(0x37, MOD_LSHIFT));
    EXPECT_EQ('?',  keymap_convert(0x38, MOD_LSHIFT));
}

TEST(test_keymap_convert_ctrl_control_chars) {
    /* Ctrl+a〜z → 制御文字（設計書 §5.4） */
    EXPECT_EQ(0x01, keymap_convert(0x04, MOD_LCTRL));  /* ^A SOH */
    EXPECT_EQ(0x03, keymap_convert(0x06, MOD_LCTRL));  /* ^C ETX */
    EXPECT_EQ(0x0D, keymap_convert(0x10, MOD_LCTRL));  /* ^M CR */
    EXPECT_EQ(0x1A, keymap_convert(0x1D, MOD_LCTRL));  /* ^Z SUB */
    EXPECT_EQ(0x01, keymap_convert(0x04, MOD_RCTRL));  /* 右Ctrl も同等 */
    /* Ctrl+記号（設計書 §5.4） */
    EXPECT_EQ(0x1F, keymap_convert(0x2D, MOD_LCTRL));  /* ^_ US */
    EXPECT_EQ(0x1B, keymap_convert(0x2F, MOD_LCTRL));  /* ^[ ESC */
    EXPECT_EQ(0x1D, keymap_convert(0x30, MOD_LCTRL));  /* ^] GS */
    EXPECT_EQ(0x1C, keymap_convert(0x31, MOD_LCTRL));  /* ^\ FS */
}

TEST(test_keymap_convert_special_keys) {
    /* 特殊キー（設計書 §5.2） */
    EXPECT_EQ(0x1B, keymap_convert(0x29, 0)); /* Esc */
    EXPECT_EQ(0x09, keymap_convert(0x2B, 0)); /* Tab */
    EXPECT_EQ(0x20, keymap_convert(0x2C, 0)); /* Space */
    EXPECT_EQ(0x08, keymap_convert(0x2A, 0)); /* Backspace → BS */
}

TEST(test_keymap_convert_keypad) {
    /* テンキー数字・小数点（設計書 §5.2） */
    EXPECT_EQ('1', keymap_convert(0x59, 0));
    EXPECT_EQ('2', keymap_convert(0x5A, 0));
    EXPECT_EQ('3', keymap_convert(0x5B, 0));
    EXPECT_EQ('4', keymap_convert(0x5C, 0));
    EXPECT_EQ('5', keymap_convert(0x5D, 0));
    EXPECT_EQ('6', keymap_convert(0x5E, 0));
    EXPECT_EQ('7', keymap_convert(0x5F, 0));
    EXPECT_EQ('8', keymap_convert(0x60, 0));
    EXPECT_EQ('9', keymap_convert(0x61, 0));
    EXPECT_EQ('0', keymap_convert(0x62, 0));
    EXPECT_EQ('.', keymap_convert(0x63, 0));
}

TEST(test_keymap_convert_priority_ctrl_over_shift) {
    /* Ctrl+Shift 同時押し → Ctrl 優先で制御文字を返す（大文字にはならない） */
    EXPECT_EQ(0x01, keymap_convert(0x04, MOD_LCTRL | MOD_LSHIFT));
    EXPECT_EQ(0x1A, keymap_convert(0x1D, MOD_LCTRL | MOD_RSHIFT));
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
    /* Phase 5 (Shift/Ctrl/特殊キー/記号/テンキー) 実装後に有効化 */
    RUN_TEST(test_keymap_convert_shift_uppercase);
    RUN_TEST(test_keymap_convert_shift_symbols);
    RUN_TEST(test_keymap_convert_symbols_no_shift);
    RUN_TEST(test_keymap_convert_symbols_with_shift);
    RUN_TEST(test_keymap_convert_ctrl_control_chars);
    RUN_TEST(test_keymap_convert_special_keys);
    RUN_TEST(test_keymap_convert_keypad);
    RUN_TEST(test_keymap_convert_priority_ctrl_over_shift);
#endif

    TEST_SUMMARY();
    return 0;
}
