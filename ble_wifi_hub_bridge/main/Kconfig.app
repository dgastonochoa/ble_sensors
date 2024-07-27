menu "BLE/WiFi hub bridge app. configuration"

    config UDP_SENSOR_SERVER_TIMEOUT
        int "UDP sensor server timeout (ms)"
        default 10000
        help
          The UDP sensor server will listen to requests for this amount of
          time in ms

endmenu
