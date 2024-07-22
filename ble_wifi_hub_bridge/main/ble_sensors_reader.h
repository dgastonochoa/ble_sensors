#ifndef BLE_SENSORS_READER_H
#define BLE_SENSORS_READER_H

#include "ble_conn_manager.h"
#include "udp_sensor_server.h"
#include "sensors_cache.h"

#define DECL_BLE_REMOTE_SENSOR(remote_ptr, sensor_id)   \
{                                                       \
    .remote = remote_ptr,                               \
    .sensor = sensor_id,                                \
    .found = false,                                     \
    .polled = false                                     \
}

struct ble_remote_sensor
{
    const struct ble_remote_dev* remote;
    enum sensor sensor;
    bool found;
    bool polled;
};

struct ble_sensors_reader
{
    struct udp_sensor_server* udp_sensor_server;
    struct ble_remote_sensor* remote_sensors;
    const size_t remote_sensors_size;
};

void ble_sensors_rd_gattc_event_handler(struct ble_gattc_app* blec,
                                       esp_gattc_cb_event_t event,
                                       esp_ble_gattc_cb_param_t* param,
                                       void* user_args);

#endif /* BLE_SENSORS_READER_H */
