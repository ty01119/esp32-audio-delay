#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "audio_delay.h"

// NVS namespace and keys
#define NVS_NAMESPACE "audio_delay"
#define NVS_KEY_DELAY_MS "delay_ms"
#define NVS_KEY_SAMPLE_RATE "sample_rate"

// Default settings
#define DEFAULT_DELAY_MS 30

typedef struct
{
    uint32_t delay_ms;
    uint32_t sample_rate;
} user_settings_t;

// Function declarations
esp_err_t settings_manager_init(void);
esp_err_t settings_manager_deinit(void);
esp_err_t settings_load(user_settings_t *settings);
esp_err_t settings_save(const user_settings_t *settings);
esp_err_t settings_reset_to_default(user_settings_t *settings);

#endif // SETTINGS_MANAGER_H
