#include "keymap.h"

/* TODO: implement in Phase 4 */
/* HID Usage ID to ASCII conversion table */
/* Supports US keyboard layout with Shift and Ctrl modifier handling */
/* Uses Boot Protocol report format as defined in USB HID spec */
/* Ctrl implementation: refer to Ctrl table in design document (設計書 section 5.4) */

uint8_t keymap_convert(uint8_t usage_id, uint8_t modifier) {
    /* TODO: implement in Phase 4 - convert HID usage_id + modifier to ASCII code */
    /* Handle: normal keys, Shift keys (uppercase/symbols), Ctrl combinations */
    (void)usage_id;
    (void)modifier;
    return 0;
}

bool keymap_is_enter(uint8_t usage_id) {
    /* TODO: implement in Phase 4 - return true for Enter (0x28) and Keypad Enter (0x58) */
    (void)usage_id;
    return false;
}
