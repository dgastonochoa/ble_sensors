#ifndef BLE_CONN_MANAGER_CONTEXT_H
#define BLE_CONN_MANAGER_CONTEXT_H

#include "ble_conn_manager.h"

struct ble_conn_manager_ctx
{
    struct ble_gattc_app** apps;
    size_t apps_cnt;
    size_t curr_prf_idx;
    bool scanning;
    bool opening;
    bool closing;
    esp_ble_scan_params_t ble_scan_params;
    struct gap_ev_functor* gap_ev_functor;
};

bool ble_conn_mngr_all_remotes_found(struct ble_conn_manager_ctx* ctx);

struct ble_remote_dev* ble_conn_mngr_get_remote_by_name(
    struct ble_conn_manager_ctx* ctx,
    const char* rem_name);

struct ble_gattc_app* ble_conn_mngr_find_profile_by_if(
    struct ble_conn_manager_ctx* ctx,
    esp_gatt_if_t gattc_if);

struct ble_gattc_app* ble_conn_mngr_next_prf(struct ble_conn_manager_ctx* ctx);

#endif /* BLE_CONN_MANAGER_CONTEXT_H */
