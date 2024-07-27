#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

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

#include "ble_conn_manager.h"
#include "ble_conn_manager_context.h"
#include "log_helpers.h"

#define TAG "CONN_MNGR"
#define BLE_MTU 500

#define ARRAY_EXPAND_6(arr) arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]
#define ARRAY_FMT_STR_6 "%02x %02x %02x %02x %02x %02x"

#define LOG_ERR_CODE(rc) ESP_LOGE(TAG, "Error %d, line %d", rc, __LINE__)

#define ERR_CHECK(rc)                                                           \
    do {                                                                        \
        if (rc != 0) {                                                          \
            LOG_ERR_CODE(rc);                                                   \
            return;                                                             \
        }                                                                       \
    } while (0)

struct ble_conn_manager_ctx ble_conn_mngr_ctx = {
    .apps = NULL,
    .apps_cnt = 0,
    .curr_prf_idx = 0,
    .scanning = false,
    .opening = false,
    .closing = false,
    .ble_scan_params = {
        .scan_type = BLE_SCAN_TYPE_ACTIVE,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval = 0x50,
        .scan_window = 0x30,
        .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE
    }
};

static esp_err_t ble_conn_mngr_gap_start_scanning(
    struct ble_conn_manager_ctx* ctx);

static esp_err_t ble_conn_mngr_gattc_open(struct ble_conn_manager_ctx* ctx,
                                          struct ble_gattc_app* app)
{
    if (ctx->opening) {
        LOG_ERR("could not open, currently opening");
        return ESP_ERR_INVALID_STATE;
    }

    if (ctx->closing) {
        LOG_ERR("could not open, currently closing");
        return ESP_ERR_INVALID_STATE;
    }

    if (ctx->scanning) {
        LOG_ERR("could not open, currently scaning");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t rc = esp_ble_gattc_open(app->gattc_if,
                                      app->target_remote->remote_addr,
                                      app->target_remote->addr_type,
                                      true);
    if (rc == ESP_OK) {
        LOG_DBG("opening app. id %d, remote %s",
                app->app_id,
                app->target_remote->name);
        ctx->opening = true;
    } else {
        LOG_ERR("could not open, error %d", rc);
    }
    return rc;
}

static esp_err_t ble_conn_mngr_gattc_close(struct ble_conn_manager_ctx* ctx,
                                           struct ble_gattc_app* app)
{
    if (ctx->opening || ctx->closing || ctx->scanning) {
        LOG_ERR("could not close, currently opening, closing or scaning");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t rc = esp_ble_gattc_close(app->gattc_if, app->virt_conn_id);
    if (rc == ESP_OK) {
        LOG_DBG("closing virtual conn. %d from prf. %d to remote %s",
                app->virt_conn_id,
                app->app_id,
                app->target_remote->name);
        ctx->closing = true;
    }
    return rc;
}

static esp_err_t ble_conn_mngr_gattc_open_next_app(
    struct ble_conn_manager_ctx* ctx)
{
    struct ble_gattc_app* next = ble_conn_mngr_next_prf(ctx);
    if (next == NULL) {
        LOG_DBG("no profiles available");
        return ESP_ERR_NOT_FOUND;

    }

    assert(next->target_remote->found);

    esp_err_t rc = ble_conn_mngr_gattc_open(ctx, next);
    if (rc != ESP_OK) {
        LOG_DBG("could not connect to %d, error %d", next->app_id, rc);
    }

    return rc;
}

static void ble_conn_mngr_gattc_handle_reg_ev(struct ble_conn_manager_ctx* ctx,
                                              struct ble_gattc_app* app,
                                              esp_ble_gattc_cb_param_t* param)
{
    esp_err_t rc = ESP_OK;
    if (ble_conn_mngr_all_remotes_found(ctx)) {
        rc = ble_conn_mngr_gattc_open_next_app(ctx);
        if (rc != ESP_OK) {
            LOG_ERR("could not open next app., error %d", rc);
        }
    } else {
        rc = ble_conn_mngr_gap_start_scanning(ctx);
        if (rc != ESP_OK) {
            LOG_ERR("could not start scannig, error %d", rc);
        }
    }
}

static void ble_conn_mngr_gattc_handle_open_ev(struct ble_conn_manager_ctx* ctx,
                                               struct ble_gattc_app* app,
                                               esp_ble_gattc_cb_param_t* param)
{
    LOG_DBG("%d, %s: OPEN, status = 0x%x",
            app->app_id,
            app->target_remote->name,
            param->open.status);

    ctx->opening = false;

    if (param->open.status != ESP_GATT_OK) {
        LOG_ERR("could not open device %s, status = 0x%x",
                 app->target_remote->name,
                 param->open.status
        );
        return;
    }

    app->virt_conn_id = param->open.conn_id;

    esp_err_t rc = esp_ble_gatt_set_local_mtu(BLE_MTU);
    if (rc != ESP_OK) {
        LOG_ERR("error %d trying to set mtu", rc);
        ble_conn_mngr_gattc_close(ctx, app);
    } else {
        LOG_DBG("local mtu set succesfully");

        rc = esp_ble_gattc_send_mtu_req(app->gattc_if, app->virt_conn_id);
        if (rc != ESP_OK) {
            LOG_ERR("error %d trying to req. config. mtu", rc);
            ble_conn_mngr_gattc_close(ctx, app);
        } else {
            LOG_DBG("local mtu request sent succesfully");
        }
    }
}

static void ble_conn_mngr_gattc_handle_cfg_mtu_ev(
    struct ble_conn_manager_ctx* ctx,
    struct ble_gattc_app* app,
    esp_ble_gattc_cb_param_t* param)
{
    ((void)ctx);

    if (param->cfg_mtu.status == ESP_GATT_OK &&
        param->cfg_mtu.mtu == BLE_MTU) {
        LOG_DBG("mtu config. as %d", BLE_MTU);
    } else {
        LOG_ERR("could not config. mtu, error %d", param->cfg_mtu.status);
        esp_err_t rc = ble_conn_mngr_gattc_close(ctx, app);
        if (rc != ESP_OK) {
            LOG_ERR("could not close connection, error %d", rc);
        }
    }
}

static void ble_conn_mngr_gattc_handle_dis_srvc_cmpl(
    struct ble_gattc_app* app,
    esp_gattc_cb_event_t event,
    esp_ble_gattc_cb_param_t* param)
{
    LOG_DBG("searching for service");

    esp_err_t rc = esp_ble_gattc_search_service(
        app->gattc_if,
        app->virt_conn_id,
        &app->target_service.uuid.id.uuid
    );
    if (rc != ESP_OK) {
        LOG_ERR("could not search service, error %d", rc);
        esp_err_t rc = ble_conn_mngr_close(app);
        if (rc != ESP_OK) {
            LOG_ERR("could not close connection, error %d", rc);
        }
    }
}

static void ble_conn_mngr_gattc_handle_srv_search_res(
    struct ble_gattc_app* app,
    esp_gattc_cb_event_t event,
    esp_ble_gattc_cb_param_t* param)
{
    esp_gatt_id_t* srvc_id = &param->search_res.srvc_id;
    if (srvc_id->uuid.len == app->target_service.uuid.id.uuid.len &&
        srvc_id->uuid.uuid.uuid16 ==
            app->target_service.uuid.id.uuid.uuid.uuid16) {

        app->target_service.found = true;
        app->target_service.start_handle = param->search_res.start_handle;
        app->target_service.end_handle = param->search_res.end_handle;

        LOG_INF("service %04x found",
                app->target_service.uuid.id.uuid.uuid.uuid16);
    } else {
        LOG_ERR("service not found in service response");
    }
}

static void ble_conn_mngr_gattc_handle_srv_search_cmpl(
    struct ble_gattc_app* app,
    esp_gattc_cb_event_t event,
    esp_ble_gattc_cb_param_t* param)
{
    LOG_DBG("service search complete");

    if (!app->target_service.found) {
        LOG_ERR("service not available");
        return;
    }

    uint16_t count = 0;
    esp_gatt_status_t rc = esp_ble_gattc_get_attr_count(
        app->gattc_if,
        app->virt_conn_id,
        ESP_GATT_DB_CHARACTERISTIC,
        app->target_service.start_handle,
        app->target_service.end_handle,
        ESP_GATT_INVALID_HANDLE,
        &count
    );

    if (rc != ESP_GATT_OK || count != 1) {
        LOG_ERR("could not get chars. count, error %d", rc);
        rc = ble_conn_mngr_close(app);
        if (rc != ESP_OK) {
            LOG_ERR("error %d trying to close connection", rc);
        }
        return;
    }

    esp_gattc_char_elem_t char_res = {0};
    rc = esp_ble_gattc_get_char_by_uuid(
        app->gattc_if,
        app->virt_conn_id,
        app->target_service.start_handle,
        app->target_service.end_handle,
        app->target_service.target_char.uuid,
        &char_res,
        &count);

    uint16_t char_uuid = app->target_service.target_char.uuid.uuid.uuid16;
    if (rc != ESP_GATT_OK || count != 1) {
        LOG_ERR("error or unexpected count (%d), error %d",
                rc,
                count);
        rc = ble_conn_mngr_close(app);
        if (rc != ESP_OK) {
            LOG_ERR("error %d trying to close connection", rc);
        }
        return;
    } else {
        LOG_INF("char. %04x found", char_uuid);
    }

    app->target_service.target_char.handle = char_res.char_handle;

    if (app != NULL && app->gattc_profile_ev_functor != NULL) {
        app->gattc_profile_ev_functor->handler(
            app, event, param, app->gattc_profile_ev_functor->user_args);
    }
}

static void ble_conn_mngr_gattc_handle_close_ev(
    struct ble_conn_manager_ctx* ctx,
    struct ble_gattc_app* app,
    esp_ble_gattc_cb_param_t* param)
{
    LOG_DBG("%d: CLOSE", app->app_id);

    ctx->closing = false;

    app->virt_conn_id = VIRT_CONN_ID_CLOSED;
    app->virt_conn_open = false;

    if (app != NULL && app->gattc_profile_ev_functor != NULL) {
        app->gattc_profile_ev_functor->handler(
            app,
            ESP_GATTC_CLOSE_EVT,
            param,
            app->gattc_profile_ev_functor->user_args);
    }

    // Notice this is here because it's expected that there will be only one
    // virtual connection (app.) per physical device. TODO Possibly move it to
    // the close handler.
    esp_err_t rc = ESP_OK;
    if (ble_conn_mngr_all_remotes_found(ctx)) {
        LOG_DBG("all remotes found, opening next app.");

        rc = ble_conn_mngr_gattc_open_next_app(ctx);
        if (rc != ESP_OK) {
            LOG_ERR("could not open next app., error %d", rc);
        }
    } else {
        rc = ble_conn_mngr_gap_start_scanning(ctx);
        if (rc != ESP_OK) {
            LOG_ERR("could not start scannig, error %d", rc);
        }
    }
}

static void ble_conn_mngr_gattc_handle_disconnect_ev(
    struct ble_conn_manager_ctx* ctx,
    struct ble_gattc_app* app,
    esp_ble_gattc_cb_param_t* param)
{
    LOG_DBG("device %s (found = %d) received a disconnect event, reason 0x%x",
            app->target_remote->name,
            app->target_remote->found ? 1 : 0,
            param->disconnect.reason);

    // It seems that the BLE stack raises this event for all apps. registered
    // when an erroneous disconnection happens. Thus, if this remote is not
    // found, there is nothing to disconnect, so ignore this event.
    //
    if (!app->target_remote->found) {
        return;
    }

    // Handle an erroneous disconnection, that is, one not performed by this
    // device.
    //
    // As explained above, this event is broadcasted in case of a failed
    // connection attempt. This means some other devices might be reachable and
    // found, and this will indeed mark them as not found and they will be
    // re-found when a discovery event triggers. There doesn't seem to be a way
    // around this.
    if (param->disconnect.reason != ESP_GATT_CONN_TERMINATE_LOCAL_HOST) {

        LOG_ERR("device %s (virtual conn. id = %d) unreachable, reason = 0x%x",
                app->target_remote->name,
                app->virt_conn_id,
                param->disconnect.reason
        );

        // If there is an error opening the BLE device (e.g. because it's
        // unreachable), the connection will disconnect without necessarily
        // going through close, because the physical connection disconnected,
        // but the virtual connection openning couldn't be stablished. Thus,
        // set opening and closing here as the close event handler has
        // potentially not being called.
        ctx->opening = false;
        ctx->closing = false;

        app->target_remote->found = false;

        esp_err_t rc = ble_conn_mngr_gap_start_scanning(ctx);
        if (rc != ESP_OK) {
            LOG_ERR("could not start scannig, error %d", rc);
        }
    }
}

static void ble_conn_mngr_gattc_cb(esp_gattc_cb_event_t event,
                                   esp_gatt_if_t gattc_if,
                                   esp_ble_gattc_cb_param_t* param)
{
    LOG_DBG("GATTC callback event %d, gattc iface. = %d", event, gattc_if);

    if (gattc_if == ESP_GATT_IF_NONE) {
        LOG_ERR("gattc if. none");
        return;
    }

    if (event == ESP_GATTC_REG_EVT && param->reg.status == ESP_GATT_OK) {
        assert(param->reg.app_id < ble_conn_mngr_ctx.apps_cnt);
        ble_conn_mngr_ctx.apps[param->reg.app_id]->app_id = param->reg.app_id;
        ble_conn_mngr_ctx.apps[param->reg.app_id]->gattc_if = gattc_if;
    }

    struct ble_gattc_app* app = ble_conn_mngr_find_profile_by_if(
        &ble_conn_mngr_ctx, gattc_if);
    if (app == NULL) {
        LOG_DBG("app. with GATTC iface. %d not found, skipping...", gattc_if);
        return;
    }
    switch (event) {
    case ESP_GATTC_REG_EVT: {
        ble_conn_mngr_gattc_handle_reg_ev(&ble_conn_mngr_ctx, app, param);
        break;
    }

    case ESP_GATTC_OPEN_EVT: {
        ble_conn_mngr_gattc_handle_open_ev(&ble_conn_mngr_ctx, app, param);
        break;
    }

    case ESP_GATTC_CFG_MTU_EVT: {
        ble_conn_mngr_gattc_handle_cfg_mtu_ev(&ble_conn_mngr_ctx, app, param);
        break;
    }

    case ESP_GATTC_DIS_SRVC_CMPL_EVT: {
        ble_conn_mngr_gattc_handle_dis_srvc_cmpl(app, event, param);
        break;
    }

    case ESP_GATTC_SEARCH_RES_EVT: {
        ble_conn_mngr_gattc_handle_srv_search_res(app, event, param);
        break;
    }

    case ESP_GATTC_SEARCH_CMPL_EVT: {
        ble_conn_mngr_gattc_handle_srv_search_cmpl(app, event, param);
        break;
    }

    case ESP_GATTC_CLOSE_EVT: {
        ble_conn_mngr_gattc_handle_close_ev(&ble_conn_mngr_ctx, app, param);
        break;
    }

    case ESP_GATTC_DISCONNECT_EVT: {
        ble_conn_mngr_gattc_handle_disconnect_ev(&ble_conn_mngr_ctx, app, param);
        break;
    }

    default: {
        LOG_DBG("unhandled GATTC event %d", event);
        if (app != NULL && app->gattc_profile_ev_functor != NULL) {
            app->gattc_profile_ev_functor->handler(
                app, event, param, app->gattc_profile_ev_functor->user_args);
        }
        break;
    }
    }
}

static esp_err_t ble_conn_mngr_gap_start_scanning(
    struct ble_conn_manager_ctx* ctx)
{
    if (ctx->scanning) {
        LOG_DBG("already scanning");
        return ESP_OK;
    }

    if (ctx->opening || ctx->closing) {
        LOG_ERR(
            "could not start scanning, busy opening or closing");
        return ESP_ERR_INVALID_STATE;
    }

    if (ble_conn_mngr_all_remotes_found(ctx)) {
        return ESP_OK;
    }

    const uint32_t duration_seconds = 3;
    esp_err_t rc = esp_ble_gap_start_scanning(duration_seconds);
    if (rc == ESP_OK) {
        LOG_DBG("starting to scan");
        ctx->scanning = true;
    }

    return rc;
}

static esp_err_t ble_conn_mngr_gap_stop_scanning(
    struct ble_conn_manager_ctx* ctx)
{
    if (!ctx->scanning) {
        LOG_DBG("already not scanning");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t rc = esp_ble_gap_stop_scanning();
    if (rc == ESP_OK) {
        LOG_DBG("stopping to scan");
        ctx->scanning = false;
    }

    return rc;
}

/*
 * Helper function to resolve a remote name from its advertising data.
 *
 */
static esp_err_t ble_conn_mngr_gap_resolve_rem_name(esp_ble_gap_cb_param_t* p,
                                                char* remote_name,
                                                size_t max_len)
{
    uint8_t adv_name_len = 0;
    uint8_t* adv_name = esp_ble_resolve_adv_data(p->scan_rst.ble_adv,
                                                 ESP_BLE_AD_TYPE_NAME_CMPL,
                                                 &adv_name_len);
    if (adv_name_len > (max_len - 1)) {
        return ESP_ERR_NO_MEM;
    }

    if (adv_name == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    memcpy(remote_name, adv_name, adv_name_len);
    remote_name[adv_name_len] = '\0';

    return ESP_OK;
}

static void gap_handle_search_inq_res(struct ble_conn_manager_ctx* ctx,
                                      esp_ble_gap_cb_param_t* param)
{
    char r_name[DEV_NAME_MAX_LEN] = {0};
    esp_err_t rc = ble_conn_mngr_gap_resolve_rem_name(
        param, r_name, DEV_NAME_MAX_LEN);
    if (rc != ESP_OK && rc != ESP_ERR_NOT_FOUND) {
        LOG_ERR("could not resolve rem., error %d", rc);
        return;
    }

    struct ble_remote_dev* rem = ble_conn_mngr_get_remote_by_name(ctx, r_name);
    if (rem == NULL) {
        LOG_DBG("no remotes found");
        return;
    }

    if (!rem->found) {
        rem->found = true;
        rem->addr_type = param->scan_rst.ble_addr_type;
        memcpy(rem->remote_addr, param->scan_rst.bda, ESP_BD_ADDR_LEN);

        LOG_INF("found remote %s, address = " ARRAY_FMT_STR_6,
                rem->name,
                ARRAY_EXPAND_6(rem->remote_addr));
    }

    if (ble_conn_mngr_all_remotes_found(ctx)) {
        rc = ble_conn_mngr_gap_stop_scanning(ctx);
        if (rc != ESP_OK) {
            LOG_ERR("could not stop scanning, error %d", rc);
        }
    }
}

static void ble_conn_mngr_gap_handle_scan_result_ev(
    esp_ble_gap_cb_param_t* param)
{
    if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
        gap_handle_search_inq_res(&ble_conn_mngr_ctx, param);
        return;
    }

    if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
        LOG_INF("search inq. completed");
        ble_conn_mngr_ctx.scanning = false;

        esp_err_t rc = ble_conn_mngr_gattc_open_next_app(&ble_conn_mngr_ctx);
        if (rc != ESP_OK) {
            if (ble_conn_mngr_ctx.gap_ev_functor != NULL) {
                LOG_DBG("could not open next app., calling GAP functor");

                ble_conn_mngr_ctx.gap_ev_functor->handler(
                    ESP_GAP_BLE_SCAN_RESULT_EVT,
                    param,
                    ble_conn_mngr_ctx.gap_ev_functor->user_args
                );
            }

            LOG_DBG(
                "could not open any connection after scanning, retrying scan");

            if (ble_conn_mngr_all_remotes_found(&ble_conn_mngr_ctx)) {
                return;
            }

            rc = ble_conn_mngr_gap_start_scanning(&ble_conn_mngr_ctx);
            if (rc != ESP_OK) {
                LOG_ERR("error trying to start scanning");
            }
        }
    }
}

static void ble_conn_mngr_gap_handle_scan_stop_ev(esp_ble_gap_cb_param_t* param)
{
    LOG_INF("scan stopped, status = %x", param->scan_stop_cmpl.status);
    ble_conn_mngr_ctx.scanning = false;

    esp_err_t rc = ble_conn_mngr_gattc_open_next_app(&ble_conn_mngr_ctx);
    if (rc != ESP_OK) {
        if (ble_conn_mngr_ctx.gap_ev_functor != NULL) {
            LOG_DBG("could not open next app., calling GAP functor");

            ble_conn_mngr_ctx.gap_ev_functor->handler(
                ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT,
                param,
                ble_conn_mngr_ctx.gap_ev_functor->user_args
            );
        }

        LOG_DBG(
            "could not open any connection after scanning, retrying scan");

        if (ble_conn_mngr_all_remotes_found(&ble_conn_mngr_ctx)) {
            return;
        }

        rc = ble_conn_mngr_gap_start_scanning(&ble_conn_mngr_ctx);
        if (rc != ESP_OK) {
            LOG_ERR("error trying to start scanning");
        }
    }
}

static void ble_conn_mngr_esp_gap_cb(esp_gap_ble_cb_event_t event,
                                     esp_ble_gap_cb_param_t* param)
{
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        LOG_INF("Set scan params complete");
        break;
    }

    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
        LOG_INF("scan started");
        break;
    }

    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        ble_conn_mngr_gap_handle_scan_result_ev(param);
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
        ble_conn_mngr_gap_handle_scan_stop_ev(param);
        break;
    }

    default:
        LOG_DBG("unhandled GAP event %d", event);
        break;
    }
}

esp_err_t ble_conn_mngr_close(struct ble_gattc_app* app)
{
    return ble_conn_mngr_gattc_close(&ble_conn_mngr_ctx, app);
}

void ble_conn_mngr_set_gap_ev_functor(struct gap_ev_functor* gap_ev_functor)
{
    ble_conn_mngr_ctx.gap_ev_functor = gap_ev_functor;
}

void ble_conn_mngr_start(struct ble_gattc_app* apps[], size_t cnt)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ret = nvs_flash_erase();
        ERR_CHECK(ret);

        ret = nvs_flash_init();
        ERR_CHECK(ret);
    }

    esp_log_level_set(TAG, LOG_LOCAL_LEVEL);

    ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    ERR_CHECK(ret);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    ERR_CHECK(ret);

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    ERR_CHECK(ret);

    ret = esp_bluedroid_init();
    ERR_CHECK(ret);

    ret = esp_bluedroid_enable();
    ERR_CHECK(ret);

    ret = esp_ble_gap_register_callback(ble_conn_mngr_esp_gap_cb);
    ERR_CHECK(ret);

    ret = esp_ble_gattc_register_callback(ble_conn_mngr_gattc_cb);
    ERR_CHECK(ret);

    ret = esp_ble_gap_set_scan_params(&ble_conn_mngr_ctx.ble_scan_params);
    ERR_CHECK(ret);

    ble_conn_mngr_ctx.apps = apps;
    ble_conn_mngr_ctx.apps_cnt = cnt;
    ble_conn_mngr_ctx.curr_prf_idx = 0;
    for (size_t i = 0; i < ble_conn_mngr_ctx.apps_cnt; i++) {
        ble_conn_mngr_ctx.apps[i]->app_id = i;
        ret = esp_ble_gattc_app_register(i);
        ERR_CHECK(ret);
    }
}
