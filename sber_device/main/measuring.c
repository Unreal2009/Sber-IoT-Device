#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_cali.h"


#include "measuring.h"

static const char *TAG = "measuring";

// Конфигурация
#define ADC_UNIT            ADC_UNIT_1
#define ADC_ATTEN           ADC_ATTEN_DB_11
#define ADC_BITWIDTH        ADC_BITWIDTH_DEFAULT
#define ADC_CHANNEL         ADC_CHANNEL_7         // GPIO 8 (ADC1_CH7)

#define SAMPLE_RATE_HZ      1000                  // 1 кГц
#define BUFFER_SIZE         10000                 // 10 секунд

// --- Глобальные переменные для работы с ADC ---
static adc_continuous_handle_t adc_handle = NULL;
static esp_adc_cal_characteristics_t adc_chars;

// Буфер: ESP‑IDF требует, чтобы он был выровнен
static uint16_t adc_buffer[BUFFER_SIZE];
static volatile size_t adc_count = 0;
static volatile bool measuring = false;
static volatile bool stop_requested = false;

static void measure_sampling_task(void *arg);

// Инициализация ADC Continuous
void measuring_init()
{
    // Конфигурация устройства
    adc_continuous_config_t adc_config = {
        .pattern_num = 1,
        .sample_freq_hz = SAMPLE_RATE_HZ,
        .conv_mode = ADC_CONTINUOUS_CONV_MODE_SINGLE_UNIT,
        .format = ADC_CONTINUOUS_OUT_FORMAT_TYPE1, // uint16_t
    };

    ESP_ERROR_CHECK(adc_continuous_new_unit(&adc_config, &adc_handle));

    // Настройка канала
    adc_continuous_channel_cfg_t ch_cfg = {
        .unit_atten = ADC_UNIT,
        .channel = ADC_CHANNEL,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_continuous_configure_channel(adc_handle, &ch_cfg));

    // Калибровка
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT, ADC_ATTEN, ADC_BITWIDTH, 0, &adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_THRESHOLD_VCC) {
        ESP_LOGW(TAG, "ADC VREF not in efuse, using default curve");
    }

    // Запускаем задачу сбора данных
    xTaskCreate(measuring_sampling_task, "adc_task", 4096, NULL, 3, NULL);

}

// Задача сбора данных (управляет непрерывным ADC)
static void measuring_sampling_task(void *arg)
{
    const size_t read_size = 512;  // читаем кусками по 512 сэмплов
    uint16_t read_buf[read_size];
    size_t total_read = 0;

    while (1) {
        // Ждём разрешения на измерение
        while (!measuring) {
            stop_requested = false;
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        adc_count = 0;
        total_read = 0;
        stop_requested = false;

        ESP_LOGI(TAG, "Starting continuous ADC measurement: rate=%d Hz, target=%zu", SAMPLE_RATE_HZ, BUFFER_SIZE);

        // 1. Старт непрерывного сбора
        ESP_ERROR_CHECK(adc_continuous_start(adc_handle, NULL));

        while (measuring && !stop_requested && (total_read < BUFFER_SIZE)) {
            size_t to_read = (BUFFER_SIZE - total_read < read_size)
                                 ? (BUFFER_SIZE - total_read)
                                 : read_size;

            adc_continuous_read_t adc_read_config = {
                .output_buffer = read_buf,
                .out_buf_size = to_read,
                .timeout_tick = pdMS_TO_TICKS(100),  // ждём до 100 мс
            };

            int32_t num_read = adc_continuous_read(adc_handle, &adc_read_config, NULL);
            if (num_read > 0) {
                // Копируем прочитанные данные в наш буфер
                for (int i = 0; i < num_read && (total_read + i) < BUFFER_SIZE; ++i) {
                    adc_buffer[total_read + i] = read_buf[i];
                }
                total_read += num_read;
            } else {
                ESP_LOGE(TAG, "ADC read failed or timeout: num_read=%d", num_read);
            }
        }

        // 2. Остановка
        adc_continuous_stop(adc_handle);

        measuring = false;
        adc_count = total_read;

        ESP_LOGI(TAG, "ADC measurement stopped: count=%zu", adc_count);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}