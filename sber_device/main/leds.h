#ifndef _LEDS_H_
#define _LEDS_H_

// Штатный светодиод на плате DevKit
#define BLINK_GPIO              48

// Внешние светодиоды - поставил, какие нашел в закромах
#define CONFIG_GPIO_LED_GREEN   4
#define CONFIG_GPIO_LED_RED     5
#define CONFIG_GPIO_LED_BLUE    6

// Автомат состояний светодиода
typedef enum {
    LED_INIT,
    LED_IDLE,
    LED_MEASURING,
    LED_WIFI_CONNECTING,
    LED_UPLOADING,
    LED_OTA_CHECKING_UPDATING,
    LED_ERROR
} led_fsm_state_t;

// Инициализация светодиодов
void leds_init();

// Выключить все светодиоды
void leds_off();

// Включение/выключение красного светодиода
void led_red_on();
void led_red_off();

// Включение/выключение зеленого светодиода
void led_green_on();
void led_green_off();

// Включение/выключение голубого светодиода
void led_blue_on();
void led_blue_off();

// Установить яркость адресного светодиода
void leds_rgb_setup(uint32_t r, uint32_t g, uint32_t b);

// Красный цвет адресного светодиода
void led_strip_red();

// Зеленый цвет адресного светодиода
void led_strip_green();

// Голубой цвет адресного светодиода
void led_strip_blue();

// Желтый цвет адресного светодиода
void led_strip_yellow();

// Желтый цвет адресного светодиода
void led_strip_off();

// Установить состояние системы для отображения светодиодами
void leds_set_fsm_state(led_fsm_state_t state);

#endif //_LEDS_H_