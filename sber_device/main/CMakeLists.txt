idf_component_register(SRCS "sber_device.c" "button.c" "leds.c" "fsm.c" "measuring.c" "csv.c"
        PRIV_REQUIRES esp_adc
        PRIV_REQUIRES vfs fatfs
        REQUIRES esp_driver_gpio
        REQUIRES driver
        INCLUDE_DIRS "."
)
