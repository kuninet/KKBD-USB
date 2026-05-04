#ifndef LED_H
#define LED_H

typedef enum {
    LED_STATE_BOOT,
    LED_STATE_WAIT_DEVICE,
    LED_STATE_MOUNTED,
    LED_STATE_TX,
    LED_STATE_ERROR
} led_state_t;

void led_init(void);
void led_set_state(led_state_t state);
void led_task(void);

#endif /* LED_H */
