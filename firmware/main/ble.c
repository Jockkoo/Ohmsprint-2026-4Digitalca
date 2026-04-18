#include "ble.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_err.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "util/ints.h"

#define TAG "ble.c"

#define MF_DATA_LEN (16)
static u8 mf_data[MF_DATA_LEN] = "4 Digitalca";
#define DEV_NAME "ESP32"
// support up to `SRVC_HANDLE_COUNT` different service things
#define SRVC_HANDLE_COUNT (64)
// subract 1 for the service handle, and devide by 3 (description, value, optional notif)
#define CHAR_COUNT ((SRVC_HANDLE_COUNT - 1) / 3)

// we need these in memory because the api requires pointers to these values
static const u16 primary_srvc_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const u16 char_decl_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const u16 notif_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

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
        .service_uuid_len = 16,
        // we are going to fill this afterwards, in `ble_init()` with the user provided data
        // .p_service_uuid = ...
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
    u16 srvc_handle;
    i32 conn_id;

    const ble_impl_t *impl;
    u16 uuid;

    ble_char_t chars[CHAR_COUNT];
    // index of the first free place in the above array
    i32 chars_count;

    // internal representation of our service
    esp_gatts_attr_db_t db[SRVC_HANDLE_COUNT];
} g;

static void
handle_adv_data_cmpl(struct ble_adv_data_cmpl_evt_param *param) {
    assert(param->status == ESP_BT_STATUS_SUCCESS);

    esp_ble_gap_start_advertising(&adv_params);
}

static void
handle_adv_start_cmpl(struct ble_adv_start_cmpl_evt_param *param) {
    assert(param->status == ESP_BT_STATUS_SUCCESS);

    ESP_LOGI(TAG, "start advertising event");
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
        default:
            // dont care
            break;
    }
}

static void
handle_reg(esp_gatt_if_t gatts_if, struct gatts_reg_evt_param *param) {
    ESP_LOGI(TAG, "reg event");
    g.gatts_if = gatts_if;

    ESP_ERROR_CHECK(esp_ble_gap_set_device_name("ESP32"));
    ESP_ERROR_CHECK(esp_ble_gap_config_adv_data(&adv_data));

    size_t next_idx = 0;
    // global service entry
    g.db[next_idx++] = (esp_gatts_attr_db_t){
            .attr_control =
                    {
                            .auto_rsp = ESP_GATT_AUTO_RSP,
                    },
            .att_desc =
                    {
                            .uuid_length = 2,
                            .uuid_p = (u8 *)&primary_srvc_uuid,
                            .perm = ESP_GATT_PERM_READ,
                            .max_length = 2,
                            .length = 2,
                            .value = (u8 *)&g.uuid,
                    },
    };

    // characteristics and their notifs
    for(size_t i = 0; i < g.chars_count; i++) {
        g.db[next_idx++] = (esp_gatts_attr_db_t){
                .attr_control =
                        {
                                .auto_rsp = ESP_GATT_AUTO_RSP,
                        },
                .att_desc =
                        {
                                .uuid_length = 2,
                                .uuid_p = (u8 *)&char_decl_uuid,
                                .perm = ESP_GATT_PERM_READ,
                                .max_length = sizeof(u8),
                                .length = sizeof(u8),
                                .value = &g.chars[i].ops,
                        },
        };

        g.db[next_idx++] = (esp_gatts_attr_db_t){
                .attr_control =
                        {
                                .auto_rsp = ESP_GATT_RSP_BY_APP,
                        },
                .att_desc =
                        {
                                .uuid_length = 2,
                                .uuid_p = (u8 *)&g.chars[i].uuid,
                                .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                .max_length = g.chars[i].size,
                                .length = 0,
                                .value = NULL,
                        },
        };

        if(g.chars[i].ops & BLE_CHAR_OP_NOTIFY) {
            g.db[next_idx++] = (esp_gatts_attr_db_t){
                    .attr_control =
                            {
                                    .auto_rsp = ESP_GATT_RSP_BY_APP,
                            },
                    .att_desc =
                            {
                                    .uuid_length = 2,
                                    .uuid_p = (u8 *)&notif_uuid,
                                    .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                    .max_length = g.chars[i].size,
                                    .length = 0,
                                    .value = NULL,
                            },
            };
        }
    }

    ESP_ERROR_CHECK(esp_ble_gatts_create_attr_tab((const esp_gatts_attr_db_t *)g.db, gatts_if, next_idx, 0));
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

    for(size_t i = 0; i < g.chars_count; i++) {
        ble_char_t *_char = &g.chars[i];

        if(_char->ops & BLE_CHAR_OP_READ && param->handle == _char->handle) {
            // ask userspace for the read, if they provide the data, then send it to the client, else the read was not
            // permitted, so we pass that to the client
            u8 *data = g.impl->read_cb(_char);
            if(data) {
                rsp.attr_value.len = _char->size;
                memcpy(rsp.attr_value.value, data, rsp.attr_value.len);

                esp_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, ESP_GATT_OK, &rsp);
            } else {
                esp_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, ESP_GATT_READ_NOT_PERMIT, &rsp);
            }
        }
    }

    ESP_LOGI(TAG, "sent read response");
}

static void
handle_write(esp_gatt_if_t gatts_if, struct gatts_write_evt_param *param) {
    ESP_LOGI(TAG, "write");

    esp_gatt_status_t status = ESP_GATT_INVALID_HANDLE;

    for(size_t i = 0; i < g.chars_count; i++) {
        ble_char_t *_char = &g.chars[i];

        if(param->handle == _char->handle) {
            if(param->len == _char->size) {
                g.impl->write_cb(&g.chars[i], param->value);
                status = ESP_GATT_OK;
            } else {
                status = ESP_GATT_INVALID_ATTR_LEN;
            }

            break;
        } else if(param->handle == _char->notif_handle && param->len == 2) {
            u32 val = param->value[1] << 8 | param->value[0];
            if(val == 0x0000) {
                g.impl->notif_cb(&g.chars[i], false);
                status = ESP_GATT_OK;
            } else if(val == 0x0001) {
                g.impl->notif_cb(&g.chars[i], true);
                status = ESP_GATT_OK;
            } else {
                status = ESP_GATT_ILLEGAL_PARAMETER;
            }

            break;
        }
    }

    if(param->need_rsp) {
        esp_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, status, NULL);
    }
}

static void
handle_exec_write(esp_gatt_if_t gatts_if, struct gatts_exec_write_evt_param *param) {
    ESP_LOGI(TAG, "exec write");

    esp_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, ESP_GATT_OK, NULL);
}

static void
handle_create_db(esp_gatt_if_t gatts_if, struct gatts_add_attr_tab_evt_param *param) {
    ESP_LOGI(TAG, "create_db event");

    g.srvc_handle = param->handles[0];

    size_t handle_idx = 1;
    for(size_t i = 0; i < g.chars_count; i++) {
        // skip the handle for the decriptions
        handle_idx++;

        // and take the next one
        g.chars[i].handle = param->handles[handle_idx++];

        if(g.chars[i].ops & BLE_CHAR_OP_NOTIFY) {
            // and the next one we have notifs for this char
            g.chars[i].notif_handle = param->handles[handle_idx++];
        }
    }

    // we should have exactly this amount of handles
    assert(handle_idx == param->num_handle);
    ESP_ERROR_CHECK(esp_ble_gatts_start_service(param->handles[0]));
}

static void
handle_connect(esp_gatt_if_t gatts_if, struct gatts_connect_evt_param *param) {
    ESP_LOGI(TAG, "connect event");
    g.conn_id = param->conn_id;

    // once we have a client we stop advertising since we only support talking to a single client
    // TODO: might allow multiple clients in the future
    ESP_ERROR_CHECK(esp_ble_gap_stop_advertising());
}

static void
handle_disconnect(esp_gatt_if_t gatts_if, struct gatts_disconnect_evt_param *param) {
    g.conn_id = -1;
    // if(g.notif_task_handle) {
    //     delete_notif();
    // }

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
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
            handle_create_db(gatts_if, &param->add_attr_tab);
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
ble_init(u16 uuid, const ble_impl_t *impl) {
    ESP_LOGI(TAG, "initializing ble...");

    g.uuid = uuid;
    g.impl = impl;

    ESP_ERROR_CHECK(init_nvs());

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t conf = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&conf));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_handler));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_handler));
}

ble_char_t *
ble_add_char(u16 uuid, ble_char_op_t ops, size_t size) {
    // there is enough space
    assert(g.chars_count != CHAR_COUNT);

    ble_char_t *_char = &g.chars[g.chars_count++];
    _char->ops = ops;
    _char->uuid = uuid;
    _char->size = size;

    return _char;
}

void
ble_start(void) {
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));
}

esp_err_t
ble_char_send_notif(ble_char_t *_char, u8 *data) {
    return esp_ble_gatts_send_indicate(g.gatts_if, g.conn_id, _char->handle, _char->size, data, false);
}
