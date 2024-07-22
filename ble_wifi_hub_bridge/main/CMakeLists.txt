idf_component_register(
    SRCS
        "app_main.c"
        "ble_conn_manager.c"
        "ble_conn_manager_context.c"
        "ble_sensors_reader.c"
        "udp_sensor_server.c"
        "sensors_cache.c"
        "atomic.c"

    INCLUDE_DIRS
        "."
)
