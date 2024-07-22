# ble_wifi_hub_bridge firmware

With this firmware, the ESP32 device will act as a BLE/WiFi bridge that reads
through BLE from a set of remote sensors (ble_edge_dev) and publishes the
results in a WiFi UDP server.

The behavior of the FW can be summarised as: read from all the remote BLE
sensors, then publish all the obtainined information over WiFi UDP for 10
seconds and start again. This FW works in a full asynchronous way, that is, it
doesn't use threads. Instead, each event performs an action that raises the
next event. BLE events are handled by actions which, eventually, will result in
running the UDP server mentioned above. When this one finishes, the BLE
event sequence is restarted.

The BLE API is already implemented in an asynchrnous fashion.

## The files that made this FW are:

 - ble_conn_manager.c/h: searches for remote sensors over BLE and reads their
 GATTC characteristics (currently limited to 1 char.). Once it obtains the char.
 value, it handles it to a user-defined functor.

 - ble_sensors_reader.c/h: implements the functor mentioned above. This module
 gathers all the char. values (sensor reads) provided by ble_conn_manager,
 stores them in a cache and initializes the WiFi UDP sensor server. The WiFi
 UDP server is initialized when all the found devices have been polled during
 this "cycle". After the UDP server finishes, all the found remotes are marked
 as unpolled and the "cycle" starts again.

 - udp_sensor_server.c/h: publishes the sensor information stored by
 ble_sensors_reader. It provides the value of a sensor given the ID of the
 latter.

 - sensors_cache.c/h: it acts as a thread-safe cache between ble_sensors_reader
 and udp_sensor_server. It's thread-safe due to old implementations based on
 threads.

 - atomic.c/h: helper module that offers atomic oprations.

 - log_helpers.h: helper module that offers log facilities.

 - ble_conn_manager_context.c/h: used by ble_conn_manager. Contains utility
 functions to search among BLE remotes etc.

 - app_main.c: declares the target BLE remotes and associates them with the
 sensor they transmit, declares the GAP and GATTC functors (more info.
 below), declares the GATTC profiles that correspond to each remote,
 instantiates the WiFi UDP server and starts the application.


## GATTC and GAP functos

As mentioned above, there aren't only GATTC functos, but GAP also. This is
because, if not all the expected remotes are found, ble_conn_manager will keep
searching for them (GAP layer). Thus, it can be desirable to be able to run the
UDP server after a GAP (not GATTC) event, as the search process consumes time.
Running the UDP server after some GAP events improves the client experience.

## Build and flash

```bash
idf.py menuconfig                       # Run Kconfig menuconfig
idf.py build                            # Build
idf.py -p $TARGET_USB flash monitor     # TARGET_USB eg.: /dev/ttyUSB0
```

## Setup remote devices

In app_main, there are several remotes declared, e.g., "ESP32-TEST-0"; these are
the target remotes. For this device to find them, one or more ESP32 devices
must be flashed with the ble_edge_dev FW, prior setting the attribute DEV_NAME
of the latter to one of the target remote names.

## Read server information

To read sensor information from the WiFi UDP server, the following command can
be used:

```bash
echo "$SENSOR_ID" | nc -u -w1 $WIFI_UDP_SEVER_IP $WIFI_UDP_SEVER_PORT;
```

The IP of the WiFi UDP serve is not fixed.

Thus, in order to find `$WIFI_UDP_SEVER_IP` and `$WIFI_UDP_SEVER_PORT`, one can
read from the ESP32 monitor if available.

Otherwise, another option is to connect to the same WiFi network with a Linux
device and use `arp` or `ip neigh`.

Another option to read the server information is to run the client (host) app.
in verbose mode. See `scripts/ble_wifi_hub_bridge_client_app/README.md`

## TODOs

 - Test with not all the expected remotes available

 - Test with remotes entering and leaving the network.
    - Bug: when a device leaves the network, the UDP server is never launched.

 - Test with remotes leaving the network until no remote is available.
