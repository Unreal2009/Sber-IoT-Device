#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "key.h"
#include "leds.h"

static const char firmware_version[] = "1.0.0";

void app_main(void)
{
    leds_init();

    key_init();

    printf("Firmware version: %s\n", firmware_version);
}
