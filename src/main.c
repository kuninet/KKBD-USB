#include "pico/stdlib.h"
#include "pico/time.h"
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

    /* Phase 2 検証用: 起動時 + 1秒毎に Ready メッセージを UART 送信。
     * Phase 3 で USB ホスト接続検出に置換予定（接続時のみ Ready 出力に変更）。
     */
    uart_out_puts("KKBD-USB Ready\r\n");
    absolute_time_t next_ready = make_timeout_time_ms(1000);

    while (true) {
        usb_host_task();
        keyrepeat_task();
        led_task();

        if (time_reached(next_ready)) {
            uart_out_puts("KKBD-USB Ready\r\n");
            next_ready = make_timeout_time_ms(1000);
        }
    }
    return 0; // unreachable
}
