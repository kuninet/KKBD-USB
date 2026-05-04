#ifndef USB_HOST_H
#define USB_HOST_H

#include <stdbool.h>

/**
 * @brief TinyUSB ホストスタックを初期化する（tuh_init() のラッパ）
 */
void usb_host_init(void);

/**
 * @brief メインループから定期的に呼び出す（tuh_task() のラッパ）
 */
void usb_host_task(void);

/**
 * @brief キーボードのマウント状態を返す
 * @return true: キーボード接続済み
 */
bool usb_host_is_keyboard_mounted(void);

/* TinyUSB HID コールバック（内部実装 - usb_host.c が提供する） */
/* tuh_hid_mount_cb(), tuh_hid_umount_cb(), tuh_hid_report_received_cb() */

#endif /* USB_HOST_H */
