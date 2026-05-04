#include "usb_host.h"
#include "tusb.h"

/* TODO: implement in Phase 3 */
/* USB Host module using TinyUSB (TUH) HID keyboard driver */
/* Handles USB device enumeration and HID keyboard reports */
/* Boot Protocol report format is assumed (CFG_TUH_HID 4, boot keyboard support) */

static bool s_keyboard_mounted = false;

void usb_host_init(void) {
    /* TODO: implement in Phase 3 - initialize TinyUSB host stack */
    /* tuh_init(BOARD_TUH_RHPORT); */
}

void usb_host_task(void) {
    /* TODO: implement in Phase 3 - call tuh_task() and process HID keyboard reports */
    /* tuh_task(); */
}

bool usb_host_is_keyboard_mounted(void) {
    return s_keyboard_mounted;
}

/* TinyUSB コールバック実装 */

/* TODO: implement in Phase 3 */
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const *desc_report, uint16_t desc_len) {
    (void)dev_addr;
    (void)instance;
    (void)desc_report;
    (void)desc_len;
    /* TODO: implement in Phase 3 - check HID_ITF_PROTOCOL_KEYBOARD, set s_keyboard_mounted */
}

/* TODO: implement in Phase 3 */
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr;
    (void)instance;
    /* TODO: implement in Phase 3 - clear s_keyboard_mounted, cancel keyrepeat, update LED */
    s_keyboard_mounted = false;
}

/* TODO: implement in Phase 3 */
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                  uint8_t const *report, uint16_t len) {
    (void)dev_addr;
    (void)instance;
    (void)report;
    (void)len;
    /* TODO: implement in Phase 3 - parse HID report, detect key events, call keymap/uart/keyrepeat */
}
