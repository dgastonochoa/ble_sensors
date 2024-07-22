#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "sensors_cache.h"
#include "ble_sensors_reader.h"
#include "log_helpers.h"

#define TAG "BLE_SENS_RDR"

static bool ble_sens_rd_all_found_sensors_polled(
    struct ble_sensors_reader* ble_sens_rd)
{
    for (size_t i = 0; i < ble_sens_rd->remote_sensors_size; i++) {
        if (ble_sens_rd->remote_sensors[i].found &&
            !ble_sens_rd->remote_sensors[i].polled) {
            return false;
        }
    }

    return true;
}

static void ble_sens_rd_mark_sensors_unpolled(
    struct ble_sensors_reader* ble_sens_rd)
{
    for (size_t i = 0; i < ble_sens_rd->remote_sensors_size; i++) {
        ble_sens_rd->remote_sensors[i].polled = false;
    }
}

static void ble_sens_rd_handle_srv_search_cmpl(struct ble_gattc_app* app,
                                               esp_gattc_cb_event_t event,
                                               esp_ble_gattc_cb_param_t* param)
{
    int rc = esp_ble_gattc_read_char(app->gattc_if,
                                     app->virt_conn_id,
                                     app->target_service.target_char.handle,
                                     ESP_GATT_AUTH_REQ_NONE);
    if (rc != ESP_GATT_OK) {
        LOG_ERR("issue read char failed, error status = %x", rc);
        ble_conn_mngr_close(app);
    }
}

static enum sensor ble_sens_rd_find_gattc_app_sensor_id(
    const struct ble_sensors_reader* ble_sens_rd,
    struct ble_gattc_app* app)
{
    for (size_t i = 0; i < ble_sens_rd->remote_sensors_size; i++) {
        if (app->target_remote == ble_sens_rd->remote_sensors[i].remote) {
            return ble_sens_rd->remote_sensors[i].sensor;
        }
    }
    return SENSOR_NONE;
}

static void ble_sens_rd_handle_read_char(
    struct ble_gattc_app* app,
    esp_gattc_cb_event_t event,
    esp_ble_gattc_cb_param_t* param,
    struct ble_sensors_reader* ble_sens_rd)
{
    if (param->read.status != ESP_GATT_OK) {
        LOG_ERR("read char failed, error status = %x", param->write.status);
        ble_conn_mngr_close(app);
        return;
    }

    sensor_val_t rd_val = { .u16 = *((uint16_t*)param->read.value) };

    enum sensor sens_id = ble_sens_rd_find_gattc_app_sensor_id(
        ble_sens_rd,
        app);

    if (sens_id >= SENSOR_NONE) {
        LOG_ERR("invalid sensor ID %d", (int)sens_id);
    } else {
        sensors_cache_set(sens_id, rd_val);

        ble_sens_rd->remote_sensors[sens_id].found = true;
        ble_sens_rd->remote_sensors[sens_id].polled = true;
    }

    LOG_INF("%s read value = %d, len = %d",
            app->target_remote->name,
            rd_val.u16,
            param->read.value_len);

    ble_conn_mngr_close(app);
}

static void ble_sens_rd_handle_close(
    struct ble_gattc_app* app,
    esp_gattc_cb_event_t event,
    esp_ble_gattc_cb_param_t* param,
    struct ble_sensors_reader* ble_sens_rd)
{
    if (!ble_sens_rd_all_found_sensors_polled(ble_sens_rd)) {
        LOG_DBG("not all found sensors polled yet");
        return;
    }

    ble_sens_rd_mark_sensors_unpolled(ble_sens_rd);

    LOG_DBG("launching UDP server");

    const uint32_t period_ms = 10000;
    udp_sensor_server_accept_requests(
        ble_sens_rd->udp_sensor_server, period_ms);
}

void ble_sensors_rd_gattc_event_handler(struct ble_gattc_app* app,
                                       esp_gattc_cb_event_t event,
                                       esp_ble_gattc_cb_param_t* param,
                                       void* user_args)
{
    struct ble_sensors_reader* us_args = (struct ble_sensors_reader*)user_args;

    switch (event) {
    case ESP_GATTC_SEARCH_CMPL_EVT: {
        ble_sens_rd_handle_srv_search_cmpl(app, event, param);
        break;
    }

    case ESP_GATTC_READ_CHAR_EVT: {
        ble_sens_rd_handle_read_char(app, event, param, us_args);
        break;
    }

    case ESP_GATTC_CLOSE_EVT: {
        ble_sens_rd_handle_close(app, event, param, us_args);
        break;
    }

    default:
        break;
    }
}
