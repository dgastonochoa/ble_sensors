#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "sdkconfig.h"
#include "adc_sensor.h"

#define DEV_NAME "ESP32-TEST-3"
#define TAG DEV_NAME

#define GATT_SERVICE_UUID 0x00FF
#define GATT_CHARACTERISTIC_UUID 0xFF01
#define GATT_HANDLE_COUNT 4

typedef struct {
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
    esp_gatt_if_t gatts_if;
    uint16_t client_write_conn;

} service_info_t;

static uint8_t adv_service_uuid128[32] = {
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0xFF,
    0x00,
    0x00,
    0x00,
};

esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static service_info_t service_def = {
    .service_id.is_primary = true,
    .service_id.id.inst_id = 0x00,
    .service_id.id.uuid.len = ESP_UUID_LEN_16,
    .service_id.id.uuid.uuid.uuid16 = GATT_SERVICE_UUID,
    .char_uuid.len = ESP_UUID_LEN_16,
    .char_uuid.uuid.uuid16 = GATT_CHARACTERISTIC_UUID,
    .descr_uuid.len = ESP_UUID_LEN_16,
    .descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,
    .gatts_if = 0,
};

static struct adc_sensor sensor = {0};

static uint16_t sensor_val = 0x001f;

static esp_attr_value_t attr_val = {
    .attr_max_len = (uint16_t)sizeof(sensor_val),
    .attr_len = (uint16_t)sizeof(sensor_val),
    .attr_value = (uint8_t*)(&sensor_val),
};

static uint16_t write_val = 24;

static void update_conn_params(esp_bd_addr_t remote_bda)
{
    esp_ble_conn_update_params_t conn_params = {0};
    memcpy(conn_params.bda, remote_bda, sizeof(esp_bd_addr_t));
    conn_params.latency = 0;
    conn_params.max_int = 0x20;
    conn_params.min_int = 0x10;
    conn_params.timeout = 400;
    esp_ble_gap_update_conn_params(&conn_params);
}

static void gap_handler(esp_gap_ble_cb_event_t event,
                        esp_ble_gap_cb_param_t* param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&adv_params);
        break;
    default:
        break;
    }
}

static int gatt_handle_write(esp_gatts_cb_event_t event,
                             esp_gatt_if_t gatts_if,
                             esp_ble_gatts_cb_param_t* param)
{
    if (service_def.descr_handle == param->write.handle) {
        uint16_t descr_value =
            param->write.value[1] << 8 | param->write.value[0];
        if (descr_value != 0x0000) {
            ESP_LOGI(TAG, "notify enable");
            esp_ble_gatts_send_indicate(gatts_if,
                                        param->write.conn_id,
                                        service_def.char_handle,
                                        attr_val.attr_len,
                                        attr_val.attr_value,
                                        false);
        } else {
            ESP_LOGI(TAG, "notify disable");
        }
        esp_ble_gatts_send_response(gatts_if,
                                    param->write.conn_id,
                                    param->write.trans_id,
                                    ESP_GATT_OK,
                                    NULL);
    } else if (service_def.char_handle == param->write.handle) {
        write_val = *((uint8_t*)param->write.value);
        ESP_LOGI(TAG, "received write request, val = %d", write_val);
        esp_ble_gatts_send_response(gatts_if,
                                    param->write.conn_id,
                                    param->write.trans_id,
                                    ESP_GATT_OK,
                                    NULL);
    } else {
        esp_ble_gatts_send_response(gatts_if,
                                    param->write.conn_id,
                                    param->write.trans_id,
                                    ESP_GATT_WRITE_NOT_PERMIT,
                                    NULL);
    }

    return 0;
}

static void gatt_handler(esp_gatts_cb_event_t event,
                         esp_gatt_if_t gatts_if,
                         esp_ble_gatts_cb_param_t* param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        esp_ble_gatts_create_service(gatts_if,
                                     &service_def.service_id,
                                     GATT_HANDLE_COUNT);
        break;

    case ESP_GATTS_CREATE_EVT:
        service_def.service_handle = param->create.service_handle;

        esp_ble_gatts_start_service(service_def.service_handle);

        esp_gatt_char_prop_t prop = ESP_GATT_CHAR_PROP_BIT_READ |
                                    ESP_GATT_CHAR_PROP_BIT_NOTIFY |
                                    ESP_GATT_CHAR_PROP_BIT_WRITE;
        esp_ble_gatts_add_char(service_def.service_handle,
                               &service_def.char_uuid,
                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                               prop,
                               &attr_val,
                               NULL);
        break;

    case ESP_GATTS_ADD_CHAR_EVT:
        service_def.char_handle = param->add_char.attr_handle;
        esp_ble_gatts_add_char_descr(service_def.service_handle,
                                     &service_def.descr_uuid,
                                     ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                     NULL,
                                     NULL);
        break;

    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        service_def.descr_handle = param->add_char_descr.attr_handle;
        esp_ble_gap_config_adv_data(&adv_data);
        break;

    case ESP_GATTS_CONNECT_EVT:
        update_conn_params(param->connect.remote_bda);
        service_def.gatts_if = gatts_if;
        service_def.client_write_conn = param->write.conn_id;
        break;

    case ESP_GATTS_READ_EVT:
        // TODO should be atomic...
        esp_gatt_rsp_t rsp = {
            .attr_value.handle = param->read.handle,
            .attr_value.len = attr_val.attr_len
        };
        memcpy(rsp.attr_value.value, attr_val.attr_value, attr_val.attr_len);

        ESP_LOGI(TAG,
                 "sending read response, val = %d",
                 *((uint16_t*)rsp.attr_value.value));

        esp_ble_gatts_send_response(gatts_if,
                                    param->read.conn_id,
                                    param->read.trans_id,
                                    ESP_GATT_OK,
                                    &rsp);
        break;

    case ESP_GATTS_WRITE_EVT:
        gatt_handle_write(event, gatts_if, param);
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        service_def.gatts_if = 0;
        esp_ble_gap_start_advertising(&adv_params);
        break;

    default:
        break;
    }
}

static void read_temp_task(void* arg)
{
    int rc = 0;
    while (1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        uint16_t res = 0;
        rc = (sensor.initialized ? adc_sensor_read(&sensor, &res) : -1);
        ESP_LOGI(TAG, "sensor read {err, val} = {%d, %d}", rc, res);

        if (service_def.gatts_if > 0 && rc == 0) {
            *((uint16_t*)attr_val.attr_value) = res;
        }
    }
}

static void notify_task(void* arg)
{
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if (service_def.gatts_if > 0) {
            esp_ble_gatts_send_indicate(service_def.gatts_if,
                                        service_def.client_write_conn,
                                        service_def.char_handle,
                                        attr_val.attr_len,
                                        attr_val.attr_value,
                                        false);
        }
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();
    esp_ble_gap_set_device_name(DEV_NAME);

    esp_ble_gap_register_callback(gap_handler);
    esp_ble_gatts_register_callback(gatt_handler);
    esp_ble_gatts_app_register(0);


    int rc = adc_sensor_init(&sensor, ADC_UNIT_1, ADC_CHANNEL_4);
    if (rc != 0) {
        ESP_LOGE(TAG, "could not init. ADC sensor");
    }

    xTaskCreate(read_temp_task,
                "temp",
                configMINIMAL_STACK_SIZE * 3,
                NULL,
                5,
                NULL);

    xTaskCreate(notify_task,
                "notify",
                configMINIMAL_STACK_SIZE * 3,
                NULL,
                6,
                NULL);
}
