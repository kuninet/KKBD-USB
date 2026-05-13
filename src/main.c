#include "pico/stdlib.h"
#include "config.h"
#include "uart_out.h"
#include "uart_monitor.h"
#include "usb_host.h"
#include "led.h"
#include "keyrepeat.h"

int main(void) {
    stdio_init_all();
    config_init();
    led_init();
    uart_out_init(config_get_baudrate());        /* UART0: SBC へキー入力送信 */
    uart_monitor_init(config_get_baudrate());    /* UART1: モニタ送信 + UART0 RX セットアップ */

    /* 起動バナー（Phase 9）。モニタ側（UART1）へ出力 */
    uart_monitor_puts("KKBD-USB v0.1 (Phase 9) - UART monitor enabled, waiting for USB keyboard...\r\n");

    usb_host_init();
    keyrepeat_init();

    while (true) {
        usb_host_task();
        keyrepeat_task();
        led_task();
        uart_monitor_task();  /* SBC 応答パススルー（UART0 RX → UART1 TX） */
    }
    return 0; /* unreachable */
}
