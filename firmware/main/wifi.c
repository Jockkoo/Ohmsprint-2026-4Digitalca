#include "wifi.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "util/ints.h"

#define WIFI_SSID "TS-DQKQ"
#define WIFI_PASS "pass"

static const char *TAG = "wifi.c";

static void
event_handler(void *arg, esp_event_base_t event_base, i32 event_id, void *event_data) {
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "disconnected... trying to connect again...");
        esp_wifi_connect();
    } else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "connected! ip address: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

esp_err_t
wifi_init(void) {
    esp_err_t err = nvs_flash_init();
    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if(err) {
            goto err;
        }
        err = nvs_flash_init();
        if(err) {
            goto err;
        }
    } else if(err) {
        goto err;
    }

    err = esp_netif_init();
    if(err) {
        goto err;
    }

    err = esp_event_loop_create_default();
    if(err) {
        goto err;
    }

    esp_netif_t *netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if(err) {
        goto err_netif;
    }

    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    if(err) {
        goto err_wifi;
    }
    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);
    if(err) {
        goto err_wifi_handler;
    }

    wifi_config_t wifi_config = {
            .sta =
                    {
                            .ssid = WIFI_SSID,
                            .password = WIFI_PASS,
                            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                    },
    };

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if(err) {
        goto err_ip_handler;
    }
    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if(err) {
        goto err_ip_handler;
    }
    err = esp_wifi_start();
    if(err) {
        goto err_ip_handler;
    }

    return ESP_OK;

err_ip_handler:
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
err_wifi_handler:
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
err_wifi:
    esp_wifi_deinit();
err_netif:
    esp_netif_destroy_default_wifi(netif);
    esp_event_loop_delete_default();
err:
    return err;
}
