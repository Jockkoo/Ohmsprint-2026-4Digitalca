#ifndef BLE_H
#define BLE_H

#include <stddef.h>

#include "esp_err.h"
#include "esp_gatt_defs.h"
#include "util/ints.h"

// this defines what options we support for this char. note that these options can be or-ed toghether, and are equal to
// the esps internal defines to make the interface easier
typedef enum ble_char_op {
    BLE_CHAR_OP_READ = ESP_GATT_CHAR_PROP_BIT_READ,
    BLE_CHAR_OP_WRITE = ESP_GATT_CHAR_PROP_BIT_WRITE,
    BLE_CHAR_OP_NOTIFY = ESP_GATT_CHAR_PROP_BIT_NOTIFY,
} ble_char_op_t;

typedef struct ble_char {
    u8 ops;
    size_t size;

    u16 uuid;
    u16 handle, notif_handle;

    // you can keep whatever you want here if you need it, e.g. a pointer to a userspace wrapper around it
    void *data;
} ble_char_t;

typedef struct ble_impl {
    // userspace needs to ensure that the data it stores in `data` return ptr is of the right size, that is, the one
    // specifed to `ble_add_char()`
    u8 *(*read_cb)(struct ble_char *_char);
    // it is guaranteed that the `data` is of the size specified by userspace
    void (*write_cb)(struct ble_char *_char, u8 *data);
    void (*notif_cb)(struct ble_char *_char, bool enabled);
} ble_impl_t;

// since there is no point in running anything without bluetooth these function just assert things work as they should

void
ble_init(u16 uuid, const ble_impl_t *impl);

ble_char_t *
ble_add_char(u16 uuid, ble_char_op_t ops, size_t size);

void
ble_start(void);

// data must be of size `_char->size` provided above
esp_err_t
ble_char_send_notif(ble_char_t *_char, u8 *data);

#endif
