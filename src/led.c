#include "led.h"

// TODO: implement in Phase 6
// LED status indicator module using Pico onboard LED (GPIO 25)
// Each state has a distinct blink pattern:
//   BOOT        - LED on solid during initialization
//   WAIT_DEVICE - slow blink (1Hz) waiting for USB keyboard
//   MOUNTED     - LED on solid, USB keyboard connected
//   TX          - brief flash on each character transmitted
//   ERROR       - fast blink (4Hz) indicating error condition

static led_state_t current_state = LED_STATE_BOOT;

void led_init(void) {
    // TODO: implement in Phase 6 - initialize GPIO 25 as output for onboard LED
}

void led_set_state(led_state_t state) {
    // TODO: implement in Phase 6 - update current state and apply initial LED output
    current_state = state;
    (void)current_state;
}

void led_task(void) {
    // TODO: implement in Phase 6 - handle timed blink patterns based on current_state
}
