set(requires esp-tls nvs_flash esp_event esp_netif esp_http_client)

idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    list(APPEND requires esp_stubs protocol_examples_common)
endif()

idf_component_register(SRCS "sber_device.c" "button.c" "leds.c" "fsm.c" "measuring.c" "csv.c" "wifi_connection.c"
        PRIV_REQUIRES esp_adc
        PRIV_REQUIRES vfs fatfs
        REQUIRES esp_driver_gpio
        REQUIRES driver
        INCLUDE_DIRS "."
        PRIV_REQUIRES ${requires}
)
