# BLE/WiFi hub brige tests

This document contains the tests for the ble_wifi device

## Test connection robustness

Note: possibly reduce the UDP server time slot for these tests

 - Connect ble_wifi
 - Connect one edge device
 - Wait
 - VERIFY: ble_wifi reads values.
 - Disconect that one edge device
 - Wait (this takes a while)
 - VERIFY: the ble_wifi bridge starts scanning again
 - Connect all edge devices at the same time
 - VERIFY: ble_wifi reads all the values
 - VERIFY: ble_wifi stops scanning
 - Disconnect one device
 - Wait (this takes a while)

 - VERIFY: ble_wifi marks the rest of devs. as disconnected, starts scanning again, founds the remaining three devs. and can read from all of them.

 - VERIFY: ble_wifi keeps scanning all the time, as there is one device to be found.
 - Connect the disconnected device.
 - VERIFY: ble_wifi finds it, reads from it and stops scanning.
 - Disconnect all devices at the same time.
 - Wait (this takes a while)
 - VERIFY: ble_wifi marks them as disconnected and starts scanning again.
 - Connect 2 edge devices at the same time.
 - VERIFY: ble_wifi finds them and reads from them.
 - Connect the remaining 2 at the same time.
 - VERIFY: ble_wifi finds them and reads from all the devices.

