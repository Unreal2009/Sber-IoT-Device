
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "led_strip.h"
#include "leds.h"

#define LED_BLINK_TIME_MS   pdMS_TO_TICKS(500)

static const char *TAG = "leds";

// Тут храним время для мигания и текущее состояние светодиода
static uint32_t last_blink = 0;
static uint32_t current_state = 0;

// Тут будем хранить текущее состояние системы для отображения светодиодами
static volatile led_fsm_state_t current_fsm_state = LED_INIT;

// Переменная работы с адресным светодиодом
static led_strip_handle_t led_strip;

static void leds_task(void *arg);

// Настройка GPIO для светодиодов
void configure_gpio_led()
{
    gpio_reset_pin(CONFIG_GPIO_LED_GREEN);
    gpio_set_direction(CONFIG_GPIO_LED_GREEN, GPIO_MODE_OUTPUT);

    gpio_reset_pin(CONFIG_GPIO_LED_RED);
    gpio_set_direction(CONFIG_GPIO_LED_RED, GPIO_MODE_OUTPUT);

    gpio_reset_pin(CONFIG_GPIO_LED_BLUE);
    gpio_set_direction(CONFIG_GPIO_LED_BLUE, GPIO_MODE_OUTPUT);
}


// Настройка адресного светодиода
void configure_leds()
{
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };

    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));

    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

void led_red_on()
{
    gpio_set_level(CONFIG_GPIO_LED_RED, 1);
}

void led_red_off()
{
    gpio_set_level(CONFIG_GPIO_LED_RED, 0);
}

void led_green_on()
{
    gpio_set_level(CONFIG_GPIO_LED_GREEN, 1);
}

void led_green_off()
{
    gpio_set_level(CONFIG_GPIO_LED_GREEN, 0);
}

void led_blue_on()
{
    gpio_set_level(CONFIG_GPIO_LED_BLUE, 1);
}

void led_blue_off()
{
    gpio_set_level(CONFIG_GPIO_LED_BLUE, 0);
}

// Выключить все светодиоды
void leds_off()
{
    led_red_off();
    led_green_off();
    led_blue_off();

    led_strip_clear(led_strip);
}

// Инициализация светодиодов
void leds_init()
{
    configure_gpio_led();
    configure_leds();

    // Запускаем задачу отображения состояния светодиодами
    xTaskCreate( leds_task, "btn_task", 2048, nullptr, 5 /* приоритет */, nullptr);
}

// Установить яркость адресного светодиода
void leds_rgb_setup(uint32_t r, uint32_t g, uint32_t b)
{
    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);
}

// Красный цвет адресного светодиода
void led_strip_red()
{
    led_strip_set_pixel(led_strip, 0, 255, 0, 0);
    led_strip_refresh(led_strip);
}

// Зеленый цвет адресного светодиода
void led_strip_green()
{
    led_strip_set_pixel(led_strip, 0, 0, 255, 0);
    led_strip_refresh(led_strip);
}

// Голубой цвет адресного светодиода
void led_strip_blue()
{
    led_strip_set_pixel(led_strip, 0, 0, 0, 255);
    led_strip_refresh(led_strip);
}

// Желтый цвет адресного светодиода
void led_strip_yellow()
{
    led_strip_set_pixel(led_strip, 0, 255, 255, 0);
    led_strip_refresh(led_strip);
}

// Желтый цвет адресного светодиода
void led_strip_off()
{
    led_strip_set_pixel(led_strip, 0, 0, 0, 0);
    led_strip_refresh(led_strip);
}

// Установить состояние системы для отображения светодиодами
void leds_set_fsm_state(led_fsm_state_t state)
{
    portMUX_TYPE my_lock = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&my_lock);

    current_fsm_state = state;
    last_blink = xTaskGetTickCount();
    current_state = 0;

    portEXIT_CRITICAL(&my_lock);
}

// Задача отображения состояния системы светодиодами
static void leds_task(void *arg)
{
    ESP_LOGI(TAG, "Leds task started");

    while (1)
    {
        switch (current_fsm_state)
        {
            case LED_INIT:
                if (last_blink < xTaskGetTickCount())
                {
                    last_blink = xTaskGetTickCount() + LED_BLINK_TIME_MS;
                    current_state = !current_state;
                    if (current_state)
                        led_strip_green();
                    else
                        led_strip_off();
                }
                break;
            case LED_IDLE:
                led_strip_green();
                break;
            case LED_MEASURING:
                if (last_blink < xTaskGetTickCount())
                {
                    last_blink = xTaskGetTickCount() + LED_BLINK_TIME_MS;
                    current_state = !current_state;
                    if (current_state)
                        led_strip_blue();
                    else
                        led_strip_off();
                }
                break;
            case LED_WIFI_CONNECTING:
                led_strip_blue();
                break;
            case LED_UPLOADING:
                if (last_blink < xTaskGetTickCount())
                {
                    last_blink = xTaskGetTickCount() + LED_BLINK_TIME_MS;
                    current_state = !current_state;
                    if (current_state)
                        led_strip_yellow();
                    else
                        led_strip_off();
                }
                break;
            case LED_OTA_CHECKING_UPDATING:
                led_strip_yellow();
                break;
            case LED_ERROR:
                if (last_blink < xTaskGetTickCount())
                {
                    last_blink = xTaskGetTickCount() + LED_BLINK_TIME_MS;
                    current_state = !current_state;
                    if (current_state)
                        led_strip_red();
                    else
                        led_strip_off();
                }
                break;
            case LED_UNDEFINED:
            default:
                led_strip_off();
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // шаг опроса 10 мс
    }
}
