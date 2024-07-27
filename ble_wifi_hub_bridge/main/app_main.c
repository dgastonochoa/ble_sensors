#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ble_sensors_reader.h"
#include "ble_conn_manager.h"

struct gap_functor_params
{
    struct udp_sensor_server* udp_srvr;
};

static struct udp_sensor_server udp_srvr = {0};

static struct ble_remote_dev remote0 = {
    .name = "ESP32-TEST-0",
    .remote_addr = {0}
};

static struct ble_remote_dev remote1 = {
    .name = "ESP32-TEST-1",
    .remote_addr = {0}
};

static struct ble_remote_dev remote2 = {
    .name = "ESP32-TEST-2",
    .remote_addr = {0}
};

static struct ble_remote_dev remote3 = {
    .name = "ESP32-TEST-3",
    .remote_addr = {0}
};

static struct ble_remote_sensor app_remote_sensors[] = {
    DECL_BLE_REMOTE_SENSOR(&remote0, SENSOR_MAGNETIC_FIELD),
    DECL_BLE_REMOTE_SENSOR(&remote1, SENSOR_PHOTOCELL),
    DECL_BLE_REMOTE_SENSOR(&remote2, SENSOR_TEMP_DETECTOR),
    DECL_BLE_REMOTE_SENSOR(&remote3, SENSOR_IR_DETECTOR)
};

static struct ble_sensors_reader ble_ev_handler_params = {
    .udp_sensor_server = &udp_srvr,
    .remote_sensors = app_remote_sensors,
    .remote_sensors_size =
        sizeof(app_remote_sensors) / sizeof(*app_remote_sensors)
};

static struct gattc_gattc_profile_ev_functor gattc_profile_ev_functor = {
    .handler = ble_sensors_rd_gattc_event_handler,
    .user_args = &ble_ev_handler_params
};

static struct gap_functor_params gap_func_params = {
    .udp_srvr = &udp_srvr
};

static void gap_ev_handler(esp_gap_ble_cb_event_t event,
                           esp_ble_gap_cb_param_t* param,
                           void* user_args)
{
    struct gap_functor_params* args = (struct gap_functor_params*)user_args;
    const uint32_t period_ms = CONFIG_UDP_SENSOR_SERVER_TIMEOUT;

    switch (event)
    {
        case ESP_GAP_BLE_SCAN_RESULT_EVT:
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            udp_sensor_server_accept_requests(args->udp_srvr, period_ms);
            break;

        default:
            break;
    }
}

static struct gap_ev_functor gap_event_functor = {
    .handler = gap_ev_handler,
    .user_args = &gap_func_params
};

// clang-format off
BLE_CON_MNGR_GATTC_PROFILE_DEFINE(prf0, &remote0, 0x00ff, 0xff01, &gattc_profile_ev_functor);
BLE_CON_MNGR_GATTC_PROFILE_DEFINE(prf1, &remote1, 0x00ff, 0xff01, &gattc_profile_ev_functor);
BLE_CON_MNGR_GATTC_PROFILE_DEFINE(prf2, &remote2, 0x00ff, 0xff01, &gattc_profile_ev_functor);
BLE_CON_MNGR_GATTC_PROFILE_DEFINE(prf3, &remote3, 0x00ff, 0xff01, &gattc_profile_ev_functor);
// clang-format on

struct ble_gattc_app* all_apps[] = {&prf0, &prf1, &prf2, &prf3};

void app_main(void) {
    udp_sensor_server_setup(&udp_srvr, CONFIG_EXAMPLE_PORT);

    ble_conn_mngr_set_gap_ev_functor(&gap_event_functor);

    ble_conn_mngr_start(all_apps, sizeof(all_apps)/ sizeof(*all_apps));
}
