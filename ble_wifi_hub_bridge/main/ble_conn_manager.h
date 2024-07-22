/**
 * @brief This module performs all the connection management logic that is
 * common to all ble apps. such as all the GAP layer logic, and the GATTC
 * connection related one.
 *
 * This module is essentialy an event loop.
 *
 */

#ifndef BLE_CONN_MANAGER_H
#define BLE_CONN_MANAGER_H

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"

#define DEV_NAME_MAX_LEN 32
#define VIRT_CONN_ID_CLOSED 0xdead

#define BLE_CON_MNGR_GATTC_PROFILE_DEFINE(var_name,                             \
                                          target_remote_ptr,                    \
                                          target_srv_uuid,                      \
                                          target_char_uuid,                     \
                                          gattc_gattc_profile_ev_functor)       \
                                                                                \
    struct ble_gattc_app var_name = {                                           \
        .target_remote = target_remote_ptr,                                     \
        .app_id = 0,                                                            \
        .virt_conn_id = VIRT_CONN_ID_CLOSED,                                    \
        .gattc_if = ESP_GATT_IF_NONE,                                           \
        .virt_conn_open = false,                                                \
        .gattc_profile_ev_functor = gattc_gattc_profile_ev_functor,             \
        .target_service = {                                                     \
            .uuid = {                                                           \
                .id = {                                                         \
                    .uuid.len = ESP_UUID_LEN_16,                                \
                    .uuid.uuid = {target_srv_uuid},                             \
                    .inst_id = 0,                                               \
                },                                                              \
                .is_primary = true,                                             \
            },                                                                  \
            .found = false,                                                     \
            .start_handle = 0,                                                  \
            .end_handle = 0,                                                    \
            .target_char = {                                                    \
                .uuid.len = ESP_UUID_LEN_16,                                    \
                .uuid.uuid = {target_char_uuid},                                \
            }                                                                   \
        },                                                                      \
    }

struct ble_gattc_app;

/**
 * @brief GATTC profile event handler.
 *
 */
typedef void (*gattc_profile_ev_handler_t)(struct ble_gattc_app* app,
                                           esp_gattc_cb_event_t event,
                                           esp_ble_gattc_cb_param_t* param,
                                           void* user_args);

/**
 * @brief GAP event handler.
 *
 */
typedef void (*gap_ev_handler_t)(esp_gap_ble_cb_event_t event,
                                 esp_ble_gap_cb_param_t* param,
                                 void* user_args);

/**
 * @brief GATTC profile event functor.
 */
struct gattc_gattc_profile_ev_functor
{
    gattc_profile_ev_handler_t handler;
    void* user_args;
};

/**
 * @brief GAP profile event functor.
 */
struct gap_ev_functor
{
    gap_ev_handler_t handler;
    void* user_args;
};

/**
 * @brief GATTC profile target remote device.
 *
 */
struct ble_remote_dev
{
    const char* name;
    esp_bd_addr_t remote_addr;
    esp_ble_addr_type_t addr_type;
    bool found;
};

/**
 * @brief GATTC characteristic.
 *
 */
struct ble_gattc_char
{
    esp_bt_uuid_t uuid;
    uint16_t handle;
};

/**
 * @brief GATTC service.
 *
 */
struct ble_gattc_service
{
    esp_gatt_srvc_id_t uuid;
    bool found;
    uint16_t start_handle;
    uint16_t end_handle;
    struct ble_gattc_char target_char;
};

/**
 * @brief GATTC application data.
 *
 */
struct ble_gattc_app
{
    struct ble_remote_dev* target_remote;
    uint16_t app_id;
    esp_gatt_if_t gattc_if;
    uint16_t virt_conn_id;
    bool virt_conn_open;
    struct ble_gattc_service target_service;
    struct gattc_gattc_profile_ev_functor* gattc_profile_ev_functor;
    struct gap_ev_functor* gap_ev_functor;
};

/**
 * @brief Provide a list of GATTC apps and start the event loop. The event
 * loop manager will iterate over these apps, handling all the connection
 * tasks and executing the app's callback when:
 *
 *  - On event ESP_GATTC_SEARCH_CMPL_EVT
 *  - On event ESP_GATTC_READ_CHAR_EVT
 *
 * All the data required by the callback to perform any BLE operation will be
 * in the parameters provided to it.
 *
 * This event loop is collaborative, so the app.'s callback is in charge to
 * disconnect (and so yield the event loop to the next app.).
 *
 * This function will keep scanning devices until all the required ones by
 * @param{apps} are found, in which case the can will stop.
 *
 * @param apps List of GATTC apps to be scheduled
 * @param cnt Number of elements in @param{apps}
 *
 */
void ble_conn_mngr_start(struct ble_gattc_app* apps[], size_t cnt);

/**
 * @brief Disconnect the given GATTC app.
 *
 * @param app GATTC app. to be disconnected.
 *
 */
esp_err_t ble_conn_mngr_close(struct ble_gattc_app* app);

/**
 * @brief Set a GAP event handler.
 *
 * @param gap_ev_functor Function to be executed upon certain GAP events:
 *      - When scan stops and there are no GATTC profiles
 *        ready to run.
 */
void ble_conn_mngr_set_gap_ev_functor(struct gap_ev_functor* gap_ev_functor);

#endif /* CONN_MANAGER_H */
