#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* MCU・OS設定（Pico SDK が提供する tusb_option.h を自動でインクルードするため不要な場合もあるが、
   明示的に指定することで一貫性を保つ） */
#define CFG_TUSB_MCU              OPT_MCU_RP2040
#define CFG_TUSB_OS               OPT_OS_PICO
#define CFG_TUSB_DEBUG            0

/* メモリ配置（Pico SDK のデフォルトを使用） */
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN        __attribute__((aligned(4)))

/* USBホストのルートハブポート番号（Pico は通常 0） */
#define BOARD_TUH_RHPORT          0

/* Host mode */
#define CFG_TUH_ENABLED           1
#define CFG_TUH_RPI_PIO_USB       0
#define CFG_TUH_HUB               0

/* HID ホストクラス: 4 インスタンス（boot keyboard 対応） */
#define CFG_TUH_HID               4

#define CFG_TUH_DEVICE_MAX        1
#define CFG_TUH_ENUMERATION_BUFSIZE 256

/* Device mode disabled */
#define CFG_TUD_ENABLED           0

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
