#ifndef SBER_DEVICE_FSM_H
#define SBER_DEVICE_FSM_H

// Состояния конечного автомата  девайса
typedef enum{
    FSM_INIT = 0,
    FSM_IDLE = 1,
    FSM_MEASURING = 2,
    FSM_WIFI_CONNECTING = 3,
    FSM_UPLOADING = 4,
    FSM_OTA_CHECKING_UPDATING = 5,
    FSM_ERROR = 6,
    FSM_UNDEFINED = 7,
} fsm_state_t;

// Установить новое состояние
void fsm_set_state(fsm_state_t new_state);

// Получить текущее состояние
fsm_state_t fsm_get_state();

#endif //SBER_DEVICE_FSM_H