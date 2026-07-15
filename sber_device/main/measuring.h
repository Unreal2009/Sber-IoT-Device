#ifndef SBER_DEVICE_MEASURING_H
#define SBER_DEVICE_MEASURING_H

// Тип данных, возвращаемый после завершения преобразования
typedef struct
{
    uint32_t data_size;     // Размер данных
    uint16_t *data;         // Данные
}measuring_data_t;

// Тип данных, возвращаемый после завершения преобразования
typedef struct
{
    uint32_t sample_rate_hz;
    uint32_t buffer_size;
}measuring_config_t;

// Инициализация ADC Continuous и задачи сбора данных
void measuring_init();

// Запуск преобразования
void measuring_start();

// Принудительная остановка преобразования
void measuring_stop();

// Получить данные после завершения преобразования
measuring_data_t measuring_get_data();

// Получить информацию о текущем количестве отсчетов
uint32_t measuring_get_current_data_size();

// Получить информацию о текущем статусе процесса
uint32_t measuring_is_runnig();

// Получить информацию о конфигурации
measuring_config_t measuring_get_config();

#endif //SBER_DEVICE_MEASURING_H