#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"

#include "ble_conn_manager_context.h"
#include "log_helpers.h"

#define TAG "CONN_MNGR_CTX"

bool ble_conn_mngr_all_remotes_found(struct ble_conn_manager_ctx* ctx)
{
    for (size_t i = 0; i < ctx->apps_cnt; i++) {
        if (!ctx->apps[i]->target_remote->found) {
            return false;
        }
    }
    return true;
}

struct ble_remote_dev* ble_conn_mngr_get_remote_by_name(
    struct ble_conn_manager_ctx* ctx,
    const char* rem_name)
{
    for (size_t i = 0; i < ctx->apps_cnt; i++) {
        if (strcmp(rem_name, ctx->apps[i]->target_remote->name) == 0) {
            return ctx->apps[i]->target_remote;
        }
    }
    return NULL;
}

struct ble_gattc_app* ble_conn_mngr_find_profile_by_if(
    struct ble_conn_manager_ctx* ctx,
    esp_gatt_if_t gattc_if)
{
    for (size_t i = 0; i < ctx->apps_cnt; i++) {
        if (ctx->apps[i]->gattc_if == gattc_if) {
            return ctx->apps[i];
        }
    }
    return NULL;
}

struct ble_gattc_app* ble_conn_mngr_next_prf(struct ble_conn_manager_ctx* ctx)
{
    for (size_t idx = ctx->curr_prf_idx + 1; idx < ctx->apps_cnt; idx++) {
        if (ctx->apps[idx]->target_remote->found) {
            LOG_DBG("next profile index = %d", idx);
            ctx->curr_prf_idx = idx;
            return ctx->apps[idx];
        }
    }

    for (size_t idx = 0; idx <= ctx->curr_prf_idx; idx++) {
        if (ctx->apps[idx]->target_remote->found) {
            LOG_DBG("next profile index = %d", idx);
            ctx->curr_prf_idx = idx;
            return ctx->apps[idx];
        }
    }

    return NULL;
}
