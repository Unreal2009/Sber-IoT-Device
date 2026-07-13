#ifndef _KEY_H_
#define _KEY_H_

// Кнопка подключена к этому пину
#define CONFIG_GPIO_KEY         47

// Конфигурация пина кнопки
void key_init();

// Получить текущее состояние кнопки
int key_get();

#endif //_KEY_H_