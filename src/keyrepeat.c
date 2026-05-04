#include "keyrepeat.h"
#include "uart_out.h"

/* TODO: implement in Phase 6 */
/* Key repeat module: generates repeated ASCII output while a key is held */
/* Timing: initial delay 500ms (KEYREPEAT_INITIAL_DELAY_MS), */
/*         repeat interval 50ms (KEYREPEAT_INTERVAL_MS) */

void keyrepeat_init(void) {
    /* TODO: implement in Phase 6 - initialize repeat state and timers */
}

void keyrepeat_register(uint8_t ascii) {
    /* TODO: implement in Phase 6 - record pressed key and start initial delay timer */
    (void)ascii;
}

void keyrepeat_cancel(void) {
    /* TODO: implement in Phase 6 - clear pressed key and cancel repeat timer */
}

void keyrepeat_task(void) {
    /* TODO: implement in Phase 6 - check elapsed time and emit repeat characters via uart_out_putc() */
}
