# ble_edge_dev firmware

Edge device from which a `ble_wifi_hub_bridge` device can read. This FW reads
from the ADC port ADCI_CH4, pin G32, where a sensor is supposed to be connected.
Once the information is read, it publishes it over BLE. The `DEV_NAME` parmeter
in `gatt_server_main.c` must be set to one of the target remote names specified
in `ble_wifi_hub_bridge`.

See `/ble_wifi_hub_bridge/README.md` for more information.
