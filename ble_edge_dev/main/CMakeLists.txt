idf_component_register(
    SRCS
        "gatt_server_main.c"
        "adc_sensor.c"
)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
