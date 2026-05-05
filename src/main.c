#include "pico/stdlib.h"
#include "config.h"
#include "uart_out.h"
#include "usb_host.h"
#include "led.h"
#include "keyrepeat.h"

int main(void) {
    stdio_init_all();
    config_init();
    led_init();
    uart_out_init(config_get_baudrate());
    usb_host_init();
    keyrepeat_init();

    /* 起動バナー（Phase 3 以降）。
     * Phase 2 の 1 秒毎 Ready ループは usb_host のマウント/アンマウントログに置換。
     */
    uart_out_puts("KKBD-USB v0.1 (Phase 5) - Waiting for USB keyboard...\r\n");

    while (true) {
        usb_host_task();
        keyrepeat_task();
        led_task();
    }
    return 0; /* unreachable */
}
