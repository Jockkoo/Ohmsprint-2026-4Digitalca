#include "main.h"

#include "ble.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "isr_mgr.h"

static struct g {
    bool enabled;
} g;

static const char *TAG = "main.c";

static void
isr_callback(pin_t *pin) {
    assert(pin->number == I_ENABLE);

    g.enabled = !g.enabled;
    gpio_set_level(O_STATE_LED, g.enabled);
}

static void
io_init(void) {
    ESP_LOGI(TAG, "initializing the io...");

    ESP_ERROR_CHECK(isr_mgr_init(isr_callback));

    // button
    ESP_ERROR_CHECK(isr_mgr_add_pin(I_ENABLE, GPIO_INTR_NEGEDGE, PULL_UP, 200));

    // led
    gpio_config_t conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << O_STATE_LED,
    };
    ESP_ERROR_CHECK(gpio_config(&conf));
}

void
app_main(void) {
    ESP_LOGI(TAG, "initializing the app...");
    io_init();
    ble_init();

    ESP_LOGI(TAG, "initializition successful!");
    gpio_set_level(O_STATE_LED, 1);

    while(1) {
        vTaskDelay(1000);
    }
}
