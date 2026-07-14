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

// Вспомогательная переменная и ф-ция для отладкт
led_fsm_state_t current_fsm_state = LED_INIT;

led_fsm_state_t get_next_fsm_state()
{
    switch (current_fsm_state)
    {
        case LED_INIT :
            current_fsm_state = LED_IDLE;
            ESP_LOGI(TAG, "LED_IDLE");
            break;
        case LED_IDLE :
            current_fsm_state = LED_MEASURING;
            ESP_LOGI(TAG, "LED_MEASURING");
            break;
        case LED_MEASURING :
            current_fsm_state = LED_WIFI_CONNECTING;
            ESP_LOGI(TAG, "LED_WIFI_CONNECTING");
            break;
        case LED_WIFI_CONNECTING :
            current_fsm_state = LED_UPLOADING;
            ESP_LOGI(TAG, "LED_UPLOADING");
            break;
        case LED_UPLOADING :
            current_fsm_state = LED_OTA_CHECKING_UPDATING;
            ESP_LOGI(TAG, "LED_OTA_CHECKING_UPDATING");
            break;
        case LED_OTA_CHECKING_UPDATING :
            current_fsm_state = LED_ERROR;
            ESP_LOGI(TAG, "LED_ERROR    ");
            break;
        case LED_ERROR :
            current_fsm_state = LED_INIT;
            ESP_LOGI(TAG, "LED_ERROR    ");
            break;
        default :
            current_fsm_state = LED_IDLE;
            break;
    }
    return current_fsm_state;
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
    //     led_strip_red();
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     led_strip_green();
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     led_strip_blue();
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     led_strip_yellow();
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     led_strip_off();
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }

    while (1)
    {
        // blink_led(s_led_color);
        //
        // s_led_color++;
        // if (s_led_color >= 3) s_led_color = 0;

        btn_event_t event = button_get_press();
        if (event == BTN_EVENT_SHORT)
        {
            // Проверяем работу светодиодов
            ESP_LOGI(TAG, "Button short press");
            leds_set_fsm_state(get_next_fsm_state());
        }else if (event == BTN_EVENT_LONG)
        {
            ESP_LOGI(TAG, "Button long press");
        }

        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }

}
