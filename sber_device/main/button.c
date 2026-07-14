
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "button.h"

// Кнопка подключена к этому пину
#define BTN_PIN                 GPIO_NUM_7

// защита от дребезга
#define DEBOUNCE_MS             50

// порог длинного нажатия (3 сек)
#define LONG_PRESS_THRESHOLD_MS 3000

static const char *TAG = "button";

static void button_task(void *arg);

// Глобальный хендл очереди (доступен из всех задач)
static QueueHandle_t btn_queue = nullptr;

// Инициализация опроса кнопки
void button_init()
{
    // Конфигурация пина кнопки - на вход с подтяжкой PULLUP
    gpio_reset_pin(BTN_PIN);
    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_PIN, GPIO_PULLUP_ONLY);

    // Создаём очередь: 1 элемент, размер каждого = sizeof(btn_event_t)
    // Можно сделать на мютексах или крит.секциях - но хочу сделать так,
    // чтобы была дальше возможность масштабирования при необходимости
    btn_queue = xQueueCreate(1, sizeof(btn_event_t));

    // Запускаем задачу опроса кнопки
    xTaskCreate( button_task, "btn_task", 2048, nullptr, 5 /* приоритет */, nullptr);
}

// Получить текущее состояние пина кнопки
int button_get_state()
{
    return gpio_get_level(BTN_PIN);
}

// Получить текущее состояние нажатия кнопки
btn_event_t button_get_press()
{
    btn_event_t received_event = BTN_EVENT_NONE;

    xQueueReceive(btn_queue, &received_event, 0);
    return received_event;
}

// Очистка очереди
static void clear_btn_queue(QueueHandle_t q)
{
    btn_event_t dummy;
    // Пока в очереди есть элементы, вычитываем их
    while (xQueueReceive(q, &dummy, 0) == pdPASS) {
        // ничего не делаем: просто удаляем элемент из очереди
    }
}

// Задача опроса кнопки
static void button_task(void *arg)
{
    bool last_stable_state = 1;      // 1 = не нажата (pull-up)
    bool current_state;
    TickType_t press_start_tick = 0;
    bool is_pressing = false;

    ESP_LOGI(TAG, "Button task started on GPIO %d", BTN_PIN);

    // Ждем пока все стабилизируется на пине кнопки
    while (button_get_state() == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(10));  // шаг опроса 10 мс
    }

    while (1)
    {
        current_state = gpio_get_level(BTN_PIN);

        // --- Антидребезг: ждём стабильного изменения ---
        if (current_state != last_stable_state)
        {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            current_state = button_get_state();  // читаем повторно
        }

        // Если состояние не изменилось после задержки — оно стабильно
        if (current_state == last_stable_state)
        {
            // Ничего не делаем: состояние стабильно
        }else
        {
            // Состояние изменилось и прошло DEBOUNCE_MS — считаем его валидным
            last_stable_state = current_state;

            if (last_stable_state == 0)
            {
                // Переход 1 → 0: кнопка нажата
                press_start_tick = xTaskGetTickCount();
                is_pressing = true;
                // Здесь можно, например, включить индикатор «кнопка нажата»
            } else if (last_stable_state == 1 && is_pressing)
            {
                // Переход 0 → 1: кнопка отпущена. Только здесь формируем событие.
                is_pressing = false;
                TickType_t press_duration_ticks = xTaskGetTickCount() - press_start_tick;
                uint32_t press_duration_ms = press_duration_ticks * portTICK_PERIOD_MS;

                btn_event_t event = BTN_EVENT_NONE;
                if (press_duration_ms >= LONG_PRESS_THRESHOLD_MS)
                {
                    ESP_LOGI(TAG, "Button long press (duration: %lu ms)", press_duration_ms);
                    event = BTN_EVENT_LONG;
                } else
                {
                    ESP_LOGI(TAG, "Button short press (duration: %lu ms)", press_duration_ms);
                    event = BTN_EVENT_SHORT;
                }
                // Чистим очередь и кладем туда тип нажатие кнопки
                clear_btn_queue(btn_queue);
                xQueueSend(btn_queue, &event, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // шаг опроса 10 мс
    }
}
