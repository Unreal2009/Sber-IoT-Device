#include "esp_log.h"

#include "leds.h"
#include "fsm.h"

static const char *TAG = "FSM";

// Текущее состояние FSM
fsm_state_t current_fsm_state = FSM_INIT;

typedef struct
{
    fsm_state_t state;
    led_fsm_state_t led_state;
    char *name;
}fsm_state_descriptor_t;

// Человекочитаемое описание состояния
static fsm_state_descriptor_t fsm_state[] =
{
    { FSM_INIT, LED_INIT, "FSM_INIT"},
    { FSM_IDLE, LED_IDLE, "FSM_IDLE"},
    { FSM_MEASURING, LED_MEASURING, "FSM_MEASURING"},
    { FSM_WIFI_CONNECTING, LED_WIFI_CONNECTING, "FSM_WIFI_CONNECTING"},
    { FSM_UPLOADING, LED_UPLOADING, "FSM_UPLOADING"},
    { FSM_OTA_CHECKING_UPDATING, LED_OTA_CHECKING_UPDATING, "FSM_OTA_CHECKING_UPDATING"},
    { FSM_ERROR, LED_ERROR, "FSM_ERROR"},
    { FSM_UNDEFINED, LED_UNDEFINED, "FSM_UNDEFINED"}
};

// Получить текстовое описание состояния
static char *fsm_get_state_string(fsm_state_t state)
{
    if(state > FSM_UNDEFINED) return "INTERNAL_ERROR";
    return fsm_state[state].name;
}

// Установить новое состояние
void fsm_set_state(fsm_state_t new_state)
{
    if(new_state > FSM_UNDEFINED)
    {
        ESP_LOGI(TAG, "Internal error (new_state=%d)", new_state);
        return;
    }

    ESP_LOGI(TAG, "%s -> %s", fsm_get_state_string(current_fsm_state), fsm_get_state_string(new_state));
    current_fsm_state = new_state;
    leds_set_fsm_state(fsm_state[current_fsm_state].led_state);
}

// Получить текущее состояние
fsm_state_t fsm_get_state()
{
    return current_fsm_state;
}
