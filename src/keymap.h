#ifndef KEYMAP_H
#define KEYMAP_H

#include <stdint.h>
#include <stdbool.h>

/* Modifier Byte のビット定義 */
#define MOD_LCTRL  (1u << 0)
#define MOD_LSHIFT (1u << 1)
#define MOD_LALT   (1u << 2)
#define MOD_LGUI   (1u << 3)
#define MOD_RCTRL  (1u << 4)
#define MOD_RSHIFT (1u << 5)
#define MOD_RALT   (1u << 6)
#define MOD_RGUI   (1u << 7)

/* 合成マクロ */
#define MOD_SHIFT  (MOD_LSHIFT | MOD_RSHIFT)
#define MOD_CTRL   (MOD_LCTRL  | MOD_RCTRL)

/**
 * @brief HID Usage ID と修飾キー状態からASCIIコードを返す
 * @param usage_id  HID Usage ID (0x04〜0x73)
 * @param modifier  Modifier Byte
 * @return ASCIIコード。変換不能時は 0x00。
 *         Enterキー（0x28）の場合も 0x00 を返し、呼び出し側で行末処理を行うこと。
 */
uint8_t keymap_convert(uint8_t usage_id, uint8_t modifier);

/**
 * @brief Enterキーかどうかを判定する
 * @param usage_id  HID Usage ID
 * @return true: Enterキー（メイン・テンキー共）
 */
bool keymap_is_enter(uint8_t usage_id);

#endif /* KEYMAP_H */
