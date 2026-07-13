#ifndef _KEY_H_
#define _KEY_H_

// --- Тип события кнопки ---
typedef enum {
    BTN_EVENT_NONE,
    BTN_EVENT_SHORT,
    BTN_EVENT_LONG
} btn_event_t;

// Инициализация опроса кнопки
void button_init();

// Получить текущее состояние пина кнопки
// Возвращает 0 - если кнопка нажата, 1 - если отпущена
int button_get_state();

// Получить текущее состояние нажатия кнопки
btn_event_t button_get_press();

#endif //_KEY_H_