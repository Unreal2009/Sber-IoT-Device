idf_component_register(SRCS "sber_device.c" "button.c" "leds.c" "fsm.c" "measuring.c"
        REQUIRES esp_adc
        REQUIRES esp_driver_gpio
        INCLUDE_DIRS "."
)
