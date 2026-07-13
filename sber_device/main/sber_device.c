/*
 * Прошивка для тестирования аппратной части - подключенных светодидов, кнопки и работы с адресным светодиодом
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "button.h"
#include "leds.h"

static const char *TAG = "example_sber_device";

// Время мигания светодиодов
#define CONFIG_BLINK_PERIOD 1000

static const char firmware_version[] = "1.0.0";


static void blink_led(uint8_t led_color)
{
    leds_off();

    switch (led_color)
    {
        case 0 :
            ESP_LOGI(TAG, "Turning the LED RED ON");
            // if (button_get_state() == 0) led_red_on();
            leds_rgb_setup(255, 0, 0);
            break;

        case 1 :
            ESP_LOGI(TAG, "Turning the LED GREEN ON");
            // if (button_get_state() == 0) led_green_on();
            leds_rgb_setup(0, 255, 0);
            break;
        case 2 :
            ESP_LOGI(TAG, "Turning the LED BLUE ON");
            // if (button_get_state() == 0) led_blue_on();
            leds_rgb_setup(0, 0, 255);
            break;
        default:
            leds_off();
            break;
    }
}

void app_main(void)
{
    static uint8_t s_led_color = 0;

    // Выводим версию прошивки девайса
    printf("Firmware version: %s\n", firmware_version);

    // Инициализируем светодиоды
    leds_init();

    // Инициализация опроса кнопки
    button_init();

    // while (1)
    // {
    //     leds_rgb_setup(255, 0, 0);
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    //     leds_rgb_setup(0, 255, 0);
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    //     leds_rgb_setup(0, 0, 255);
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    //     leds_rgb_setup(255, 255, 255);
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    //     leds_rgb_setup(0, 0, 0);
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    // }

    while (1)
    {
        blink_led(s_led_color);

        s_led_color++;
        if (s_led_color >= 3) s_led_color = 0;

        btn_event_t event = button_get_press();
        if (event == BTN_EVENT_SHORT)
        {
            ESP_LOGI(TAG, "Button short press");
        }else if (event == BTN_EVENT_LONG)
        {
            ESP_LOGI(TAG, "Button long press");
        }

        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }

}
