#include "keymap.h"

/* Phase 5 実装範囲: 設計書 §5.1〜§5.5 のテーブル仕様を完全実装。
 *
 * テーブル方式（O(1) アクセス）:
 *   - s_normal_table: 修飾キーなし時の変換（§5.2）
 *   - s_shift_table : Shift 押下時の変換（§5.3）
 *   - s_ctrl_table  : Ctrl 押下時の変換（§5.4）
 *
 * Enter / Keypad Enter (0x28 / 0x58) は keymap_is_enter() で別判定し、
 * keymap_convert() では 0 を返す（呼び出し側で uart_out_send_line_ending を呼ぶ）。
 *
 * 修飾キー優先度: Ctrl > Shift > Normal（標準的挙動。Ctrl+Shift+a → ^A）。
 *
 * 既知の制約（Phase 7 以降で対応検討）:
 *   - NumLock 状態追跡なし。Keypad キー（0x59-0x63）は常に数字として送信
 *   - Alt/GUI 修飾キーは未対応（Phase 4 と同様、無視）
 */

/* 通常テーブル（設計書 §5.2） */
static const uint8_t s_normal_table[256] = {
    /* 英字 a-z */
    [0x04] = 'a', [0x05] = 'b', [0x06] = 'c', [0x07] = 'd',
    [0x08] = 'e', [0x09] = 'f', [0x0A] = 'g', [0x0B] = 'h',
    [0x0C] = 'i', [0x0D] = 'j', [0x0E] = 'k', [0x0F] = 'l',
    [0x10] = 'm', [0x11] = 'n', [0x12] = 'o', [0x13] = 'p',
    [0x14] = 'q', [0x15] = 'r', [0x16] = 's', [0x17] = 't',
    [0x18] = 'u', [0x19] = 'v', [0x1A] = 'w', [0x1B] = 'x',
    [0x1C] = 'y', [0x1D] = 'z',
    /* 数字 1-9, 0 */
    [0x1E] = '1', [0x1F] = '2', [0x20] = '3', [0x21] = '4',
    [0x22] = '5', [0x23] = '6', [0x24] = '7', [0x25] = '8',
    [0x26] = '9', [0x27] = '0',
    /* Enter (0x28) は 0 のまま（呼び出し側で keymap_is_enter 判定） */
    /* 特殊キー */
    [0x29] = 0x1B, /* Esc */
    [0x2A] = 0x08, /* Backspace -> BS */
    [0x2B] = 0x09, /* Tab */
    [0x2C] = 0x20, /* Space */
    /* 記号キー */
    [0x2D] = '-',  [0x2E] = '=',
    [0x2F] = '[',  [0x30] = ']', [0x31] = '\\',
    [0x33] = ';',  [0x34] = '\'', [0x35] = '`',
    [0x36] = ',',  [0x37] = '.',  [0x38] = '/',
    /* Keypad Enter (0x58) は 0 のまま（keymap_is_enter 判定） */
    /* Keypad 1-9, 0, . （NumLock ON 想定。NumLock 連動は将来対応） */
    [0x59] = '1', [0x5A] = '2', [0x5B] = '3', [0x5C] = '4',
    [0x5D] = '5', [0x5E] = '6', [0x5F] = '7', [0x60] = '8',
    [0x61] = '9', [0x62] = '0', [0x63] = '.',
};

/* Shift テーブル（設計書 §5.3） */
static const uint8_t s_shift_table[256] = {
    /* Shift+英字 → 大文字 A-Z */
    [0x04] = 'A', [0x05] = 'B', [0x06] = 'C', [0x07] = 'D',
    [0x08] = 'E', [0x09] = 'F', [0x0A] = 'G', [0x0B] = 'H',
    [0x0C] = 'I', [0x0D] = 'J', [0x0E] = 'K', [0x0F] = 'L',
    [0x10] = 'M', [0x11] = 'N', [0x12] = 'O', [0x13] = 'P',
    [0x14] = 'Q', [0x15] = 'R', [0x16] = 'S', [0x17] = 'T',
    [0x18] = 'U', [0x19] = 'V', [0x1A] = 'W', [0x1B] = 'X',
    [0x1C] = 'Y', [0x1D] = 'Z',
    /* Shift+数字 → 記号 */
    [0x1E] = '!', [0x1F] = '@', [0x20] = '#', [0x21] = '$',
    [0x22] = '%', [0x23] = '^', [0x24] = '&', [0x25] = '*',
    [0x26] = '(', [0x27] = ')',
    /* Shift+記号キー → Shift時記号 */
    [0x2D] = '_',  [0x2E] = '+',
    [0x2F] = '{',  [0x30] = '}', [0x31] = '|',
    [0x33] = ':',  [0x34] = '"', [0x35] = '~',
    [0x36] = '<',  [0x37] = '>', [0x38] = '?',
};

/* Ctrl テーブル（設計書 §5.4） */
static const uint8_t s_ctrl_table[256] = {
    /* Ctrl+a-z → 制御文字 0x01-0x1A */
    [0x04] = 0x01, [0x05] = 0x02, [0x06] = 0x03, [0x07] = 0x04,
    [0x08] = 0x05, [0x09] = 0x06, [0x0A] = 0x07, [0x0B] = 0x08,
    [0x0C] = 0x09, [0x0D] = 0x0A, [0x0E] = 0x0B, [0x0F] = 0x0C,
    [0x10] = 0x0D, [0x11] = 0x0E, [0x12] = 0x0F, [0x13] = 0x10,
    [0x14] = 0x11, [0x15] = 0x12, [0x16] = 0x13, [0x17] = 0x14,
    [0x18] = 0x15, [0x19] = 0x16, [0x1A] = 0x17, [0x1B] = 0x18,
    [0x1C] = 0x19, [0x1D] = 0x1A,
    /* Ctrl+記号 */
    [0x2D] = 0x1F, /* ^_ -> US */
    [0x2F] = 0x1B, /* ^[ -> ESC */
    [0x30] = 0x1D, /* ^] -> GS */
    [0x31] = 0x1C, /* ^\ -> FS */
};

uint8_t keymap_convert(uint8_t usage_id, uint8_t modifier) {
    /* Enter は呼び出し側で行末コード処理されるため 0 を返す */
    if (keymap_is_enter(usage_id)) {
        return 0;
    }

    bool const ctrl  = (modifier & MOD_CTRL)  != 0;
    bool const shift = (modifier & MOD_SHIFT) != 0;

    /* 優先度: Ctrl > Shift > Normal（Ctrl+Shift+a → ^A） */
    if (ctrl) {
        return s_ctrl_table[usage_id];
    }
    if (shift) {
        return s_shift_table[usage_id];
    }
    return s_normal_table[usage_id];
}

bool keymap_is_enter(uint8_t usage_id) {
    /* Return Enter (0x28) と Keypad Enter (0x58) を Enter として扱う */
    return (usage_id == 0x28) || (usage_id == 0x58);
}
