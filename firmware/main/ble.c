#include "ble.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_err.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "util/ints.h"

#define TAG "ble.c"

#define MF_DATA_LEN (16)
static u8 mf_data[MF_DATA_LEN] = "OhmSprint";

#define DEV_NAME "ESP32"

#define SRVC_HANDLE_COUNT (4)

#define SRVC_UUID_LEN (2)
// for some reason different parts of the api could not agree on a single format, so you have to keep both, but the
// first one is little endian, fuck it
static u8 srvc_uuid[SRVC_UUID_LEN] = {0x00, 0xFF};
#define SRVC_UUID (0xFF00)
#define READ_CHAR_UUID (0xFF01)

static esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = false,
        .min_interval = ESP_BLE_GAP_CONN_ITVL_MS(7.5),  // slave connection min interval
        .max_interval = ESP_BLE_GAP_CONN_ITVL_MS(20),  // slave connection max interval
        .appearance = 0x00,
        .manufacturer_len = MF_DATA_LEN,
        .p_manufacturer_data = mf_data,
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = SRVC_UUID_LEN * 8,
        .p_service_uuid = (u8 *)&srvc_uuid,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
        .adv_int_min = ESP_BLE_GAP_ADV_ITVL_MS(20),
        .adv_int_max = ESP_BLE_GAP_ADV_ITVL_MS(40),
        .adv_type = ADV_TYPE_IND,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static struct g {
    esp_gatt_if_t gatts_if;
    u16 service_handle;
    u16 char_handle;
    u16 notify_handle;
    i32 conn_id;

    TaskHandle_t notif_task_handle;
} g;

static void
handle_adv_data_cmpl(struct ble_adv_data_cmpl_evt_param *param) {
    assert(param->status == ESP_BT_STATUS_SUCCESS);

    esp_ble_gap_start_advertising(&adv_params);
}

static void
handle_adv_start_cmpl(struct ble_adv_start_cmpl_evt_param *param) {
    assert(param->status == ESP_BT_STATUS_SUCCESS);

    ESP_LOGI(TAG, "advertising started successfully");
}

static void
handle_update_conn_params(struct ble_update_conn_params_evt_param *param) {
    ESP_LOGI(TAG, "connection params updated: status %d, min_int %d, max_int %d", param->status, param->min_int,
            param->max_int);
}

static void
gap_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch(event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            handle_adv_data_cmpl(&param->adv_data_cmpl);
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            handle_adv_start_cmpl(&param->adv_start_cmpl);
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            handle_update_conn_params(&param->update_conn_params);
            break;
        default:
            // dont care
            break;
    }
}

static void
handle_reg(esp_gatt_if_t gatts_if, struct gatts_reg_evt_param *param) {
    g.gatts_if = gatts_if;

    // todo: should this go here?
    ESP_ERROR_CHECK(esp_ble_gap_set_device_name("ESP32"));
    ESP_ERROR_CHECK(esp_ble_gap_config_adv_data(&adv_data));

    esp_gatt_srvc_id_t srvc_id = {
            .is_primary = true,
            .id =
                    {
                            .inst_id = 0x00,  // this is always 0, useless
                            .uuid = {.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = SRVC_UUID}},
                    },
    };

    ESP_ERROR_CHECK(esp_ble_gatts_create_service(gatts_if, &srvc_id, SRVC_HANDLE_COUNT));
}

static void
handle_read(esp_gatt_if_t gatts_if, struct gatts_read_evt_param *param) {
    ESP_LOGI(TAG, "read event");

    if(!param->need_rsp) {
        // dont care
        return;
    }
    esp_gatt_rsp_t rsp = {0};
    rsp.attr_value.handle = param->handle;

    const char *status = "WE UP!";
    rsp.attr_value.len = strlen(status);
    memcpy(rsp.attr_value.value, status, rsp.attr_value.len);

    esp_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, ESP_GATT_OK, &rsp);
    ESP_LOGI(TAG, "sent read response");
}

static void
task(void *arg) {
    while(1) {
        u16 val = (uint16_t)(6500 + (esp_random() % 1001));
        ESP_ERROR_CHECK(
                esp_ble_gatts_send_indicate(g.gatts_if, g.conn_id, g.char_handle, sizeof(val), (u8 *)&val, false));
        vTaskDelay(200);
    }
}

static void
setup_notif(void) {
    ESP_LOGI(TAG, "notifications enabled");

    assert(xTaskCreate(task, "handle_notif", 4096, NULL, 1, &g.notif_task_handle));
}

static void
delete_notif(void) {
    ESP_LOGI(TAG, "notifications disabled");

    vTaskDelete(g.notif_task_handle);
    g.notif_task_handle = NULL;
}

static void
handle_write(esp_gatt_if_t gatts_if, struct gatts_write_evt_param *param) {
    if(param->handle == g.notify_handle && param->len == 2) {
        // enabled notifications
        u16 descr_value = param->value[1] << 8 | param->value[0];
        if(descr_value == 0x0001) {
            setup_notif();
        } else if(descr_value == 0x0000) {
            delete_notif();
        }
    }

    if(param->need_rsp) {
        esp_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, ESP_GATT_OK, NULL);
    }
}

static void
handle_exec_write(esp_gatt_if_t gatts_if, struct gatts_exec_write_evt_param *param) {
    esp_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, ESP_GATT_OK, NULL);
}

static void
handle_mtu(esp_gatt_if_t gatts_if, struct gatts_mtu_evt_param *param) {
    ESP_LOGI(TAG, "mtu exchange, mtu size: %d", param->mtu);
}

static void
handle_create(esp_gatt_if_t gatts_if, struct gatts_create_evt_param *param) {
    assert(param->status == ESP_GATT_OK);

    g.service_handle = param->service_handle;
    ESP_ERROR_CHECK(esp_ble_gatts_start_service(g.service_handle));

    // add characteristics
    esp_bt_uuid_t uuid = {
            .len = ESP_UUID_LEN_16,
            .uuid = {.uuid16 = READ_CHAR_UUID},
    };
    ESP_ERROR_CHECK(esp_ble_gatts_add_char(g.service_handle, &uuid, ESP_GATT_PERM_READ,
            ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY, NULL, NULL));
}

static void
handle_add_char(esp_gatt_if_t gatts_if, struct gatts_add_char_evt_param *param) {
    assert(param->status == ESP_GATT_OK);

    g.char_handle = param->attr_handle;

    esp_bt_uuid_t uuid = {
            .len = ESP_UUID_LEN_16,
            // ovaj uuid mora da bude ovaj da bi ga protokol prepoznao kao notify
            .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG},
    };

    ESP_ERROR_CHECK(esp_ble_gatts_add_char_descr(g.service_handle, &uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
            NULL, NULL));
}

static void
handle_add_char_descr(esp_gatt_if_t gatts_if, struct gatts_add_char_descr_evt_param *param) {
    assert(param->status == ESP_GATT_OK);
    g.notify_handle = param->attr_handle;
}

static void
handle_connect(esp_gatt_if_t gatts_if, struct gatts_connect_evt_param *param) {
    g.conn_id = param->conn_id;
}

static void
handle_disconnect(esp_gatt_if_t gatts_if, struct gatts_disconnect_evt_param *param) {
    g.conn_id = -1;
    if(g.notif_task_handle) {
        delete_notif();
    }

    // start advertising again
    ESP_ERROR_CHECK(esp_ble_gap_start_advertising(&adv_params));
}

static void
gatts_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch(event) {
        case ESP_GATTS_REG_EVT:
            handle_reg(gatts_if, &param->reg);
            break;
        case ESP_GATTS_READ_EVT:
            handle_read(gatts_if, &param->read);
            break;
        case ESP_GATTS_WRITE_EVT:
            handle_write(gatts_if, &param->write);
            break;
        case ESP_GATTS_EXEC_WRITE_EVT:
            handle_exec_write(gatts_if, &param->exec_write);
            break;
        case ESP_GATTS_MTU_EVT:
            handle_mtu(gatts_if, &param->mtu);
            break;
        case ESP_GATTS_CREATE_EVT:
            handle_create(gatts_if, &param->create);
            break;
        case ESP_GATTS_ADD_CHAR_EVT:
            handle_add_char(gatts_if, &param->add_char);
            break;
        case ESP_GATTS_ADD_CHAR_DESCR_EVT:
            handle_add_char_descr(gatts_if, &param->add_char_descr);
            break;
        case ESP_GATTS_CONNECT_EVT:
            handle_connect(gatts_if, &param->connect);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            handle_disconnect(gatts_if, &param->disconnect);
            break;
        default:
            break;
    }
}

static esp_err_t
init_nvs() {
    esp_err_t err = nvs_flash_init();
    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if(err) {
            return err;
        }
        err = nvs_flash_init();
    }

    return err;
}

void
ble_init(void) {
    ESP_LOGI(TAG, "initializing ble...");

    ESP_ERROR_CHECK(init_nvs());

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_handler));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_handler));

    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));

    // todo: check how to do this
    // ESP_ERROR_CHECK(esp_ble_gatt_set_local_mtu(MTU));
}
