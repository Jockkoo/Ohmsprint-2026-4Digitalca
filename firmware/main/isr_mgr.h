#ifndef ISR_MGR_H
#define ISR_MGR_H

#include "esp_err.h"
#include "hal/gpio_types.h"
#include "util/ints.h"

#define ISR_MGR_EVENT_QUEUE_SIZE 64

typedef struct pin {
    int number;

    // for debouncing
    int debounce_delay_ms;
    u64 last_intr_us;
} pin_t;

typedef void (*isr_callback_t)(pin_t *pin);

typedef enum pull {
    PULL_NONE = 0,
    PULL_UP,
    PULL_DOWN,
} pull_t;

esp_err_t
isr_mgr_init(isr_callback_t callback);

// setting `debounce_delay_ms` to 0 disables it
esp_err_t
isr_mgr_add_pin(int pin_number, gpio_int_type_t intr, pull_t pull, int debounce_delay_ms);

#endif
