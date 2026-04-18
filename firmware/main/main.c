#include "main.h"

#include "ble.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "isr_mgr.h"
#include "util/macros.h"

#define SRVC_UUID (0xFF00)
#define CURRENT_UUID (0xFF01)
#define VOLTAGE_UUID (0xFF02)

static struct g {
    ble_char_t *current, *voltage;

    u32 notif_period_ms;

    u32 current_value, voltage_value;
    TaskHandle_t current_notif, voltage_notif;
} g;

static const char *TAG = "main.c";

static void
isr_callback(pin_t *pin) {
    // do nothing, left for demonstration
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

static u8 *
handle_read(struct ble_char *_char) {
    ESP_LOGI(TAG, "got read");

    if(_char == g.current) {
        return (u8 *)&g.current_value;
    } else if(_char == g.voltage) {
        return (u8 *)&g.voltage_value;
    }

    return NULL;
}

static void
handle_write(struct ble_char *_char, u8 *data) {
    ESP_LOGI(TAG, "got write");
}

static void
current_notif_task(void *arg) {
    UNUSED(arg);

    while(1) {
        ble_char_send_notif(g.current, (u8 *)&g.current_value);
        vTaskDelay(g.notif_period_ms / portTICK_PERIOD_MS);
    }
}

static void
voltage_notif_task(void *arg) {
    UNUSED(arg);

    while(1) {
        ble_char_send_notif(g.voltage, (u8 *)&g.voltage_value);
        vTaskDelay(g.notif_period_ms / portTICK_PERIOD_MS);
    }
}

static void
handle_notif(struct ble_char *_char, bool enabled) {
    ESP_LOGI(TAG, "got notif");

    if(_char == g.current) {
        // dont enable it twice, as that would leek memory
        if(enabled && !g.current_notif) {
            if(!xTaskCreate(current_notif_task, "current_notif", 4096, NULL, 1, &g.current_notif)) {
                ESP_LOGE(TAG, "could not setup current notif");
            }
        } else if(!enabled && g.current_notif) {
            vTaskDelete(g.current_notif);
            g.current_notif = NULL;
        }
    } else if(_char == g.voltage) {
        // dont enable it twice, as that would leek memory
        if(enabled && !g.voltage_notif) {
            if(!xTaskCreate(voltage_notif_task, "voltage_notif", 4096, NULL, 1, &g.voltage_notif)) {
                ESP_LOGE(TAG, "could not setup voltage notif");
            }
        } else if(!enabled && g.voltage_notif) {
            vTaskDelete(g.voltage_notif);
            g.voltage_notif = NULL;
        }
    }
}

static const ble_impl_t ble_impl = {
        .read_cb = handle_read,
        .write_cb = handle_write,
        .notif_cb = handle_notif,
};

void
app_main(void) {
    ESP_LOGI(TAG, "initializing the app...");
    io_init();

    g.current_value = 69;
    g.voltage_value = 420;
    g.notif_period_ms = 1000;

    // initialize the ble subsystem, and add two chars to it. char (characteristic) is a single piece of data we want
    // to comunicate to the client, or we want the client to write to. they can also receive notifications. char is
    // recognized by its uuid (universally unique identifier), as well as the service. we add two: one for the current
    // sensor and one for the voltage sensor. you can add more as you please by specifying the uuid of the char which
    // events are supported for that char, and whats its size.
    ble_init(SRVC_UUID, &ble_impl);
    g.current = ble_add_char(CURRENT_UUID, BLE_CHAR_OP_READ | BLE_CHAR_OP_NOTIFY, sizeof(u32));
    g.voltage = ble_add_char(VOLTAGE_UUID, BLE_CHAR_OP_READ | BLE_CHAR_OP_NOTIFY, sizeof(u32));
    ble_start();

    ESP_LOGI(TAG, "initializition successful!");
    gpio_set_level(O_STATE_LED, 1);

    while(1) {
        vTaskDelay(1000);
    }
}
