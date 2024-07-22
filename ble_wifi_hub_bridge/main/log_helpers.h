#ifndef LOG_HELPERS_H
#define LOG_HELPERS_H

#define LOG_ERR(...) ESP_LOGE(TAG, __VA_ARGS__)
#define LOG_WRN(...) ESP_LOGW(TAG, __VA_ARGS__)
#define LOG_INF(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOG_DBG(...) ESP_LOGD(TAG, __VA_ARGS__)

#endif /* LOG_HELPERS_H */