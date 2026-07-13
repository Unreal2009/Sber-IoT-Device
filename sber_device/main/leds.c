
#include "driver/gpio.h"
#include "led_strip.h"
#include "leds.h"

// Переменная работы с адресным светодиодом
static led_strip_handle_t led_strip;

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
}

// Установить яркость адресного светодиода
void leds_rgb_setup(uint32_t r, uint32_t g, uint32_t b)
{
    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);
}
