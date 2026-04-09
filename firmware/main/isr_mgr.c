#include "isr_mgr.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "util/ints.h"

static struct g {
    isr_callback_t callback;
    pin_t pins[64];  // map of pins on the chip
    QueueHandle_t event_queue_handle;
    TaskHandle_t task_handle;
} g;

static void IRAM_ATTR
gpio_isr_handler(void *arg) {
    int pin_number = (int)arg;

    xQueueSendFromISR(g.event_queue_handle, &pin_number, NULL);
}

static void
task_handle_events(void *arg) {
    int pin_number;

    while(1) {
        if(xQueueReceive(g.event_queue_handle, &pin_number, portMAX_DELAY)) {
            pin_t *pin = &g.pins[pin_number];

            u64 now = esp_timer_get_time();
            if(pin->debounce_delay_ms > 0 && now - pin->last_intr_us < pin->debounce_delay_ms * 1000) {
                continue;
            }

            g.callback(pin);
            g.pins[pin_number].last_intr_us = now;
        }
    }
}

esp_err_t
isr_mgr_init(isr_callback_t callback) {
    g.callback = callback;

    esp_err_t err = ESP_FAIL;

    g.event_queue_handle = xQueueCreate(ISR_MGR_EVENT_QUEUE_SIZE, sizeof(int));
    if(!g.event_queue_handle) {
        goto err;
    }

    // start the task that will react to events, using the default values
    int success = xTaskCreate(task_handle_events, "handle_events", 4096, NULL, 3, &g.task_handle);
    if(!success) {
        goto err_queue;
    }

    err = gpio_install_isr_service(0);
    if(err) {
        goto err_task;
    }

    return ESP_OK;

err_task:
    vTaskDelete(g.task_handle);
err_queue:
    vQueueDelete(g.event_queue_handle);
err:
    return err;
}

esp_err_t
isr_mgr_add_pin(int pin_number, gpio_int_type_t intr, pull_t pull, int debounce_delay_ms) {
    gpio_config_t conf = {
            .mode = GPIO_MODE_INPUT,
            .pin_bit_mask = 1ULL << pin_number,
            .intr_type = intr,
            .pull_up_en = pull == PULL_UP,
            .pull_down_en = pull == PULL_DOWN,
    };

    esp_err_t err = gpio_config(&conf);
    if(err) {
        return err;
    }

    err = gpio_isr_handler_add(pin_number, gpio_isr_handler, (void *)pin_number);
    if(err) {
        return err;
    }

    // setup our state for the pin
    g.pins[pin_number] = (pin_t){
            .number = pin_number,
            .debounce_delay_ms = debounce_delay_ms,
    };

    return ESP_OK;
}
