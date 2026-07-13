
#include "driver/gpio.h"
#include "key.h"

// Конфигурация пина кнопки - на вход с подтяжкой PULLUP
void key_init()
{
    gpio_reset_pin(CONFIG_GPIO_KEY);
    gpio_set_direction(CONFIG_GPIO_KEY, GPIO_MODE_INPUT);
    gpio_set_pull_mode(CONFIG_GPIO_KEY, GPIO_PULLUP_ONLY);
}

// Получить текущее состояние кнопки
int key_get()
{
    return gpio_get_level(CONFIG_GPIO_KEY);
}
