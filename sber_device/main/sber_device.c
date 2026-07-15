/*
 * Проект на базе ESP32-S3 + FreeRTOS для получения данных с АЦП и передачи их на сервер
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "button.h"
#include "leds.h"
#include "fsm.h"
#include "measuring.h"

static const char *TAG = "Sber_Device";

static const char firmware_version[] = "1.0.0";

void show_init()
{
    ESP_LOGI(TAG, "SYSTEM START");

    // Выводим версию прошивки девайса
    ESP_LOGI(TAG, "Firmware version: %s", firmware_version);
}

void app_main(void)
{
    measuring_init();

    while (1)
    {
        measuring_start();
        for (int i = 0; i < 15; i++)
        {
            ESP_LOGI(TAG, "Data size is %"PRIu32, measuring_get_current_data_size());
            vTaskDelay(pdMS_TO_TICKS(1000));
            if (!measuring_is_runnig()) break;
        }
        measuring_stop();

        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    // Инициализируем светодиоды
    leds_init();

    // Показываем процесс инициализации
    show_init();

    // Инициализация опроса кнопки
    button_init();

    vTaskDelay(pdMS_TO_TICKS(1000));

    // Основной цикл работы
    // Не стал выносить в отдельную задачу - пусть крутится тут
    while (1)
    {
        // Проверяем нажата ли кнопка
        btn_event_t event = button_get_press();

        switch (fsm_get_state())
        {
            case FSM_INIT:
                fsm_set_state(FSM_IDLE);
                break;
            
            case FSM_IDLE:
                if (event == BTN_EVENT_SHORT)
                {
                    fsm_set_state(FSM_MEASURING);
                }else if (event == BTN_EVENT_LONG)
                {
                    fsm_set_state(FSM_OTA_CHECKING_UPDATING);
                }
                break;

            case FSM_MEASURING :
                for (int i = 0; i < 1000; i++)
                {
                    vTaskDelay(pdMS_TO_TICKS(10));
                    event = button_get_press();
                    if (event == BTN_EVENT_SHORT)
                    {
                        ESP_LOGI(TAG, "Measure stoped by user");
                        break;
                    }
                }
                fsm_set_state(FSM_WIFI_CONNECTING);
                break;

            case FSM_WIFI_CONNECTING :
                vTaskDelay(pdMS_TO_TICKS(2000));
                fsm_set_state(FSM_UPLOADING);
                break;

            case FSM_UPLOADING :
                vTaskDelay(pdMS_TO_TICKS(3000));
                fsm_set_state(FSM_IDLE);
                break;

            case FSM_OTA_CHECKING_UPDATING :
                fsm_set_state(FSM_IDLE);
                break;

            case FSM_UNDEFINED :
            case FSM_ERROR :
                if (event == BTN_EVENT_SHORT)
                {
                    fsm_set_state(FSM_IDLE);
                }
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

}
