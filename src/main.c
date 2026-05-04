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

    while (true) {
        usb_host_task();
        keyrepeat_task();
        led_task();
    }
    return 0; // unreachable
}
