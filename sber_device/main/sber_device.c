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
#include "csv.h"

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

    // Инициализируем светодиоды
    leds_init();

    // Показываем процесс инициализации
    show_init();

    // Инициализация опроса кнопки
    button_init();

    // Инициализация модуля измерения
    measuring_init();

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
                    measuring_start();
                    fsm_set_state(FSM_MEASURING);
                }else if (event == BTN_EVENT_LONG)
                {
                    fsm_set_state(FSM_OTA_CHECKING_UPDATING);
                }
                break;

            case FSM_MEASURING :
                // Юзер хочет остановить измерение
                if (event == BTN_EVENT_SHORT)
                {
                    measuring_stop();
                }

                // Измерение закончено
                // Делаем CSV файл и выводим превью
                // Потом переходим к отправке на сервер
                if (!measuring_is_runnig())
                {
                    csv_create_file(measuring_get_data());
                    csv_preview();

                    fsm_set_state(FSM_WIFI_CONNECTING);
                    break;
                }
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
