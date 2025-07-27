#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "ec11_encoder.h"
#include "oled_display.h"
#include "settings_manager.h"

// UI states
typedef enum
{
    UI_STATE_MAIN,        // Main delay adjustment
    UI_STATE_MENU,        // Sample rate menu
    UI_STATE_MENU_CONFIRM // Confirming sample rate selection
} ui_state_t;

// Sample rate options
typedef enum
{
    SAMPLE_RATE_44K = 0,
    SAMPLE_RATE_48K,
    SAMPLE_RATE_96K,
    SAMPLE_RATE_192K,
    SAMPLE_RATE_COUNT
} sample_rate_option_t;

typedef struct
{
    ui_state_t current_state;
    oled_display_t display;
    user_settings_t settings;
    sample_rate_option_t selected_sample_rate;
    bool settings_changed;
    uint32_t last_interaction_time;
} ui_manager_t;

// Function declarations
esp_err_t ui_manager_init(ui_manager_t *ui);
esp_err_t ui_manager_deinit(ui_manager_t *ui);
void ui_manager_handle_encoder_event(ui_manager_t *ui, ec11_event_t event);
esp_err_t ui_manager_update_display(ui_manager_t *ui);
esp_err_t ui_manager_save_settings(ui_manager_t *ui);
uint32_t ui_manager_get_sample_rate_value(sample_rate_option_t option);
sample_rate_option_t ui_manager_get_sample_rate_option(uint32_t sample_rate);

// Helper functions
uint32_t ui_manager_get_current_delay(ui_manager_t *ui);
uint32_t ui_manager_get_current_sample_rate(ui_manager_t *ui);
bool ui_manager_settings_changed(ui_manager_t *ui);

#endif // UI_MANAGER_H
