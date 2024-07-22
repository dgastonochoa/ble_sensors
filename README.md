# ble_sensors

## Firmware

This repository contains the FW for the BLE edge devices (sensors) and for the
BLE/WiFi hub bridge device. Each of the above are in separated directories:

 - ble_edge_dev: contains the FW for the edge device
 - ble_wifi_bug_bridge: contains the FW for the hub bridge device

Each of the FW images are built separately as standard espress-idf projects.

To build each FW project:
```
cd $PROJECT_DIR
source ../esp-idf/export.sh
idf.py set-target esp32
idf.py menuconfig               # OPTIONAL, run Kconfig menuconfig
idf.py build
```

To flash:
```
idf.py -p $TARGET_DEVICe flash monitor
```
where `$TARGET_DEVICe` can be something like `/dev/ttyUSB0`.

See also README files inside each directory.

## Client application

The client application reads (over UDP) from the BLE/WiFi bridge and publishes
the results in an HTTP server.

See also the README file in `ble_sensors_client_app`
