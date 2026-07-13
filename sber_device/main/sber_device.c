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

static uint8_t s_led_state = 0;

static uint8_t s_led_color = 0;

static void blink_led(uint8_t led_state)
{
    leds_off();

    if (s_led_color == 0)
    {
        // ESP_LOGI(TAG, "Turning the LED RED ON");
        if (button_get_state() == 0) led_red_on();
        leds_rgb_setup(255, 0, 0);
    }else if (s_led_color == 1)
    {
        // ESP_LOGI(TAG, "Turning the LED GREEN ON");
        if (button_get_state() == 0) led_green_on();
        leds_rgb_setup(0, 255, 0);
    }else if (s_led_color == 2)
    {
        // ESP_LOGI(TAG, "Turning the LED BLUE ON");
        if (button_get_state() == 0) led_blue_on();
        leds_rgb_setup(0, 0, 255);
    }
}

void app_main(void)
{
    // Выводим версию прошивки девайса
    printf("Firmware version: %s\n", firmware_version);

    // Инициализируем светодиоды
    leds_init();

    // Инициализация опроса кнопки
    button_init();

    while (1) {
        // ESP_LOGI(TAG, "Turning the LED %s! key %d", s_led_state == true ? "ON" : "OFF", button_get_state());
        blink_led(s_led_state);
        /* Toggle the LED state */
        s_led_state = !s_led_state;

        s_led_color++;
        if (s_led_color > 3) s_led_color = 0;

        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);

        btn_event_t event = button_get_press();
        if (event == BTN_EVENT_SHORT)
        {
            ESP_LOGI(TAG, "Button short press");
        }else if (event == BTN_EVENT_LONG)
        {
            ESP_LOGI(TAG, "Button long press");
        }
    }

}
