/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <errno.h>

#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "udp_sensor_server.h"
#include "sensors_cache.h"
#include "log_helpers.h"

static const char *TAG = "UDP_SRVR";

static int udp_sensor_server_get_socket(struct udp_sensor_server* udp_srvr)
{
    udp_srvr->sever_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_srvr->sever_sock_addr.sin_family = AF_INET;
    udp_srvr->sever_sock_addr.sin_port = htons(udp_srvr->port);

    udp_srvr->sock = socket(
        udp_srvr->sever_sock_addr.sin_family, SOCK_DGRAM, IPPROTO_IP);
    if (udp_srvr->sock < 0) {
        return udp_srvr->sock;
    }

    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = udp_srvr->timeout_us;
    setsockopt(
        udp_srvr->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int err = bind(udp_srvr->sock,
                   (struct sockaddr *)&udp_srvr->sever_sock_addr,
                   sizeof(udp_srvr->sever_sock_addr));
    if (err < 0) {
        return err;
    }

    return udp_srvr->sock;
}

static int udp_sensor_server_wait_for_request(struct udp_sensor_server* udp_srvr)
{
    socklen_t socklen = sizeof(udp_srvr->client_sock_addr);

    int recv_bytes = recvfrom(udp_srvr->sock,
                              udp_srvr->rx_buffer,
                              sizeof(udp_srvr->rx_buffer) - 1,
                              0,
                              &udp_srvr->client_sock_addr,
                              &socklen);
    if (recv_bytes < 0) {
        return recv_bytes;
    }

    udp_srvr->rx_buffer[recv_bytes] = '\0';
    return recv_bytes;
}

static int udp_sensor_server_handle_request(struct udp_sensor_server* udp_srvr)
{
    uint8_t code = udp_srvr->rx_buffer[0];
    code -= (uint8_t)'0';

    sensor_val_t val = {0};
    enum sensor sens_id = (enum sensor)code;

    if (sens_id >= SENSOR_NONE) {
        LOG_ERR("Invalid sensor ID %c", udp_srvr->rx_buffer[0]);
    } else {
        sensors_cache_get(sens_id, &val);
    }

    char response_str[32] = {0};
    snprintf(response_str,
             sizeof(response_str),
             "sensor_value=%d\n",
             val.u16);

    return sendto(udp_srvr->sock,
                  response_str,
                  sizeof(response_str),
                  0,
                  &udp_srvr->client_sock_addr,
                  sizeof(udp_srvr->client_sock_addr));
}

static void udp_sensor_server_close_socket(struct udp_sensor_server* udp_srvr)
{
    if (udp_srvr->sock == -1) {
        return;
    }

    shutdown(udp_srvr->sock, 0);
    close(udp_srvr->sock);
    udp_srvr->sock = -1;
}

void udp_sensor_server_accept_requests(
    struct udp_sensor_server* udp_srvr, uint32_t period_ms)
{
    int rc = udp_sensor_server_get_socket(udp_srvr);

    if (rc < 0) {
        LOG_ERR("Unable to create socket: error %d", udp_srvr->sock);
        return;
    }

    TickType_t startTick = xTaskGetTickCount();
    const TickType_t timeout = pdMS_TO_TICKS(period_ms);

    while ((xTaskGetTickCount() - startTick) <= timeout) {
        LOG_DBG("waiting for requests - [elapsed = %lu, timeout  = %lu]",
                pdTICKS_TO_MS((xTaskGetTickCount() - startTick)),
                pdTICKS_TO_MS(timeout));

        int len = udp_sensor_server_wait_for_request(udp_srvr);

        if (len < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERR("recvfrom failed, error %d", errno);
                break;
            } else {
                continue;
            }
        }

        rc = udp_sensor_server_handle_request(udp_srvr);

        if (rc < 0) {
            LOG_ERR("Error occurred during sending: errno %d", errno);
            break;
        }
    }

    udp_sensor_server_close_socket(udp_srvr);
}

void udp_sensor_server_setup(struct udp_sensor_server* udp_srvr, uint16_t port)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in
     * menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    udp_srvr->sock = -1;
    udp_srvr->timeout_us = 300e3; // TODO Avoid the eneed for this
    udp_srvr->port = port;
}
