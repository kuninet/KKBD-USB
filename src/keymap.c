#include "keymap.h"

/* Phase 4 実装範囲:
 *   - 英字 a-z (HID Usage ID 0x04 - 0x1D)
 *   - 数字 0-9 (HID Usage ID 0x1E - 0x27)
 *   - Enter / Keypad Enter 判定
 *
 * Phase 5 で追加:
 *   - Shift 修飾（大文字・記号）
 *   - Ctrl 修飾（制御文字 0x01-0x1A）
 *   - 特殊キー（Esc, Tab, Space, Backspace）
 *   - 記号キー全般（- = [ ] \ ; ' ` , . / 等）
 *
 * 参考: 設計書 §5.2（通常テーブル）、§5.3（Shiftテーブル）、§5.4（Ctrlテーブル）
 */

uint8_t keymap_convert(uint8_t usage_id, uint8_t modifier) {
    /* Phase 4: modifier は使用しない。Phase 5 で Shift/Ctrl 修飾を実装する */
    (void)modifier;

    /* 英字 a-z: HID 0x04 (a) 〜 0x1D (z) */
    if (usage_id >= 0x04 && usage_id <= 0x1D) {
        return (uint8_t)('a' + (usage_id - 0x04));
    }
    /* 数字 1-9: HID 0x1E (1) 〜 0x26 (9) */
    if (usage_id >= 0x1E && usage_id <= 0x26) {
        return (uint8_t)('1' + (usage_id - 0x1E));
    }
    /* 数字 0: HID 0x27 */
    if (usage_id == 0x27) {
        return (uint8_t)'0';
    }
    /* 未対応キー（Phase 5 で記号・特殊キーを追加） */
    return 0;
}

bool keymap_is_enter(uint8_t usage_id) {
    /* Return Enter (0x28) と Keypad Enter (0x58) を Enter として扱う */
    return (usage_id == 0x28) || (usage_id == 0x58);
}
