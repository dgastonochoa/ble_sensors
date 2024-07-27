#ifndef UDP_SENSOR_SERVER_H
#define UDP_SENSOR_SERVER_H

#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>

struct udp_sensor_server
{
    int sock;
    char rx_buffer[128];
    struct sockaddr_in sever_sock_addr;
    struct sockaddr client_sock_addr;
    time_t timeout_us;
    uint16_t port;
};

/**
 * @brief Setup an UDP server to listen on port @p port. This function only
 * sets up the server; call @ref udp_sensor_server_accept_requests to listen
 * for requests.
 *
 */
void udp_sensor_server_setup(struct udp_sensor_server* udp_srvr, uint16_t port);

/**
 * @brief Blocking function. Accept UDP requests to read sensor values during
 * @p period_ms milliseconds.
 *
 */
void udp_sensor_server_accept_requests(
    struct udp_sensor_server* udp_srvr, uint32_t period_ms);

#endif /* UDP_SENSOR_SERVER_H */
