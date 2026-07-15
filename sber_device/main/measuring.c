/*
 * SPDX-FileCopyrightText: 2021-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"

#include "measuring.h"

#define MEASURING_ADC_UNIT                  ADC_UNIT_1
#define MEASURING_ADC_CONV_MODE             ADC_CONV_SINGLE_UNIT_1
#define MEASURING_ADC_ATTEN                 ADC_ATTEN_DB_12
#define MEASURING_ADC_BIT_WIDTH             SOC_ADC_DIGI_MAX_BITWIDTH
#define MEASURING_FREQ_HZ                   1000

#define MEASURING_READ_LEN                  256

#define MEASURING_BUFFER_SIZE               10000 // Размер основного буфера для сбора и хранения данных

static adc_channel_t channel[1] = {ADC_CHANNEL_7};

static TaskHandle_t s_task_handle;
static const char *TAG = "measuring";

// Буфер для хранения данных
static uint16_t measuring_buffer[MEASURING_BUFFER_SIZE];

// Структура для хранения информации о данных
measuring_data_t measuring_data =
{
    .data_size = 0,
    .data = measuring_buffer
};

// Флажок - запуск преобразования
static volatile uint32_t measuring_running = 0;

// Таска для обеспечения логики работы
static void measuring_task(void *arg);

// Callback, вызываемый при заполнении внутреннего буфера DMA при готовности данных
// Основной поток получает информацию - что можно парсить данные от ADC
static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}

// Инициализация ADC и режима DMA
static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = MEASURING_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = MEASURING_FREQ_HZ,
        .conv_mode = MEASURING_ADC_CONV_MODE,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten = MEASURING_ADC_ATTEN;
        adc_pattern[i].channel = channel[i] & 0x7;
        adc_pattern[i].unit = MEASURING_ADC_UNIT;
        adc_pattern[i].bit_width = MEASURING_ADC_BIT_WIDTH;

        ESP_LOGI(TAG, "adc_pattern[%d].atten is :%"PRIx8, i, adc_pattern[i].atten);
        ESP_LOGI(TAG, "adc_pattern[%d].channel is :%"PRIx8, i, adc_pattern[i].channel);
        ESP_LOGI(TAG, "adc_pattern[%d].unit is :%"PRIx8, i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}

// Запуск преобразования
void measuring_start()
{
    portMUX_TYPE my_lock = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&my_lock);

    // Чистим размер данных и разрешаем запуск выборок
    measuring_data.data_size = 0;
    measuring_running = 1;

    portEXIT_CRITICAL(&my_lock);
}

// Принудительная остановка преобразования
void measuring_stop()
{
    portMUX_TYPE my_lock = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&my_lock);

    measuring_running = 0;

    portEXIT_CRITICAL(&my_lock);
}

// Получить данные после завершения преобразования
measuring_data_t measuring_get_data()
{
    return measuring_data;
}

// Получить информацию о текущем количестве отсчетов
uint32_t measuring_get_current_data_size()
{
    uint32_t size = 0;

    portMUX_TYPE my_lock = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&my_lock);

    size = measuring_data.data_size;

    portEXIT_CRITICAL(&my_lock);

    return size;
}

// Получить информацию о текущем статусе процесса
uint32_t measuring_is_runnig()
{
    uint32_t is_running = 0;

    portMUX_TYPE my_lock = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&my_lock);

    is_running = measuring_running;

    portEXIT_CRITICAL(&my_lock);

    return is_running;
}

void measuring_init()
{
    // Запускаем задачу работы с ADC
    xTaskCreate( measuring_task, "measuring_task", 8 * 1024, nullptr, 5 /* приоритет */, nullptr);
}

// Задача опроса ADC
static void measuring_task(void *arg)
{
    // Инициализация ADC для работы в режиме 'continuous'
    // Используется ADC_CHANNEL_7 (GPIO_8)
    ESP_LOGI(TAG, "Measuring task started");

    esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[MEASURING_READ_LEN] = {0};
    memset(result, 0xcc, MEASURING_READ_LEN);

    s_task_handle = xTaskGetCurrentTaskHandle();

    adc_continuous_handle_t handle = NULL;
    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle);

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));

    while (1)
    {
        // Можно сделать на мьютексах или семафорах - но сейчас пусть будут флажки
        // Ждем разрешения запуска
        while (!measuring_running)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        ESP_ERROR_CHECK(adc_continuous_start(handle));

        while (measuring_running)
        {

            /**
             * This is to show you the way to use the ADC continuous mode driver event callback.
             * This `ulTaskNotifyTake` will block when the data processing in the task is fast.
             * However in this example, the data processing (print) is slow, so you barely block here.
             *
             * Without using this event callback (to notify this task), you can still just call
             * `adc_continuous_read()` here in a loop, with/without a certain block timeout.
             */
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            while (1)
            {
                ret = adc_continuous_read(handle, result, MEASURING_READ_LEN, &ret_num, 0);
                if (ret == ESP_OK) {
                    // ESP_LOGI("TASK", "ret is %x, ret_num is %"PRIu32" bytes", ret, ret_num);

                    adc_continuous_data_t parsed_data[ret_num / SOC_ADC_DIGI_RESULT_BYTES];
                    uint32_t num_parsed_samples = 0;

                    esp_err_t parse_ret = adc_continuous_parse_data(handle, result, ret_num, parsed_data, &num_parsed_samples);
                    if (parse_ret == ESP_OK) {
                        measuring_data.data_size += num_parsed_samples;
                        for (int i = 0; i < num_parsed_samples; i++) {
                            if (parsed_data[i].valid) {
                                // ESP_LOGI(TAG, "ADC%d, Channel: %d, Value: %"PRIu32,
                                //          parsed_data[i].unit + 1,
                                //          parsed_data[i].channel,
                                //          parsed_data[i].raw_data);
                            } else {
                                ESP_LOGW(TAG, "Invalid data [ADC%d_Ch%d_%"PRIu32"]",
                                         parsed_data[i].unit + 1,
                                         parsed_data[i].channel,
                                         parsed_data[i].raw_data);
                            }
                        }
                    } else {
                        ESP_LOGE(TAG, "Data parsing failed: %s", esp_err_to_name(parse_ret));
                    }

                    /**
                     * Because printing is slow, so every time you call `ulTaskNotifyTake`, it will immediately return.
                     * To avoid a task watchdog timeout, add a delay here. When you replace the way you process the data,
                     * usually you don't need this delay (as this task will block for a while).
                     */
                    vTaskDelay(1);
                } else if (ret == ESP_ERR_TIMEOUT) {
                    //We try to read `EXAMPLE_READ_LEN` until API returns timeout, which means there's no available data
                    break;
                }
            }
        }

        ESP_ERROR_CHECK(adc_continuous_stop(handle));
    }
    // Сюда никогда не попадаем - т.к. требований по экономии ресурсов нет
    ESP_ERROR_CHECK(adc_continuous_deinit(handle));
}
