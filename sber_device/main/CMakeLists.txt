idf_component_register(SRCS "sber_device.c" "key.c" "leds.c"
        INCLUDE_DIRS "."
        REQUIRES led_strip drivers
)
