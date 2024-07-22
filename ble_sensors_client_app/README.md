# ble_wifi_hub_bridge_client_app

This is a Python host application that reads sensor information from
`ble_wifi_hub_bridge` over UDP and publishes it through an HTTP server, in
plain HTML. Run it with -v to see debug traces.

## The files that compose it are:

 - ble_wifi_hub_bridge_client_app.py: application module. Run it with -h to
 obtain help. Run it providing target UDP serve IP and port, and HTTP serve IP
 and port, or run it with default values. Once running, the sensor information
 can be accessed through a web browser. It instantiates the UDP server to read
 the sensor information (in a thread) and publishes it in a HTTP server (main
 thread).

 - udp_client.py: reads from the WiFi UDP sensor server at the provided target
 IP and port and stores the read values in a thread-safe cache. It performs
 UDP requests in a loop for a certain time, then sleeps for a different amount
 time.

 - http_server.py: runs constantly and publishes the information read by
 udp_client through a thread-safe cache.

 - atomic.py: helper module that provides atomic operations.


## Tests

The directpry `test` contains `ble_wifi_hub_bridge_udp_server_sim.py`, which
is a UDP server that aims to simulate the real `ble_wifi_hub_bridge`. It's
important this 'simulator' characterizes the `ble_wifi_hub_bridge` behavior; to
achieve this, it must provide information for a period of time, then sleep for a
different period and start again. It listens on localhost:3333; no arguments are
required to run it.
