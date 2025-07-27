#include "ui_manager.h"
#include "audio_delay.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "UI_MANAGER";

// Auto-save timeout (5 seconds of inactivity)
#define AUTO_SAVE_TIMEOUT_MS 5000

// Sample rate mapping
static const uint32_t sample_rate_values[SAMPLE_RATE_COUNT] = {
    44100, // SAMPLE_RATE_44K
    48000, // SAMPLE_RATE_48K
    96000, // SAMPLE_RATE_96K
    192000 // SAMPLE_RATE_192K
};

esp_err_t ui_manager_init(ui_manager_t *ui)
{
    if (!ui)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize UI state
    ui->current_state = UI_STATE_MAIN;
    ui->selected_sample_rate = SAMPLE_RATE_48K;
    ui->settings_changed = false;
    ui->last_interaction_time = esp_timer_get_time() / 1000; // Convert to ms

    // Load settings from NVS
    ESP_ERROR_CHECK(settings_load(&ui->settings));

    // Initialize OLED display
    ESP_ERROR_CHECK(oled_display_init(&ui->display));

    // Update display with loaded settings
    ESP_ERROR_CHECK(oled_display_update_delay(&ui->display, ui->settings.delay_ms));
    ESP_ERROR_CHECK(oled_display_update_sample_rate(&ui->display, ui->settings.sample_rate));

    // Set initial sample rate selection based on loaded settings
    ui->selected_sample_rate = ui_manager_get_sample_rate_option(ui->settings.sample_rate);

    // Show main display
    ESP_ERROR_CHECK(oled_display_show_main(&ui->display));

    ESP_LOGI(TAG, "UI manager initialized");
    return ESP_OK;
}

esp_err_t ui_manager_deinit(ui_manager_t *ui)
{
    if (!ui)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Save settings if changed
    if (ui->settings_changed)
    {
        settings_save(&ui->settings);
    }

    // Deinitialize display
    ESP_ERROR_CHECK(oled_display_deinit(&ui->display));

    ESP_LOGI(TAG, "UI manager deinitialized");
    return ESP_OK;
}

void ui_manager_handle_encoder_event(ui_manager_t *ui, ec11_event_t event)
{
    if (!ui)
    {
        return;
    }

    ui->last_interaction_time = esp_timer_get_time() / 1000; // Update interaction time

    switch (ui->current_state)
    {
    case UI_STATE_MAIN:
        switch (event)
        {
        case EC11_CW:
            // Increase delay
            if (ui->settings.delay_ms < MAX_DELAY_MS)
            {
                ui->settings.delay_ms += DELAY_STEP_MS;
                if (ui->settings.delay_ms > MAX_DELAY_MS)
                {
                    ui->settings.delay_ms = MAX_DELAY_MS;
                }
                oled_display_update_delay(&ui->display, ui->settings.delay_ms);
                ui->settings_changed = true;
            }
            break;

        case EC11_CCW:
            // Decrease delay
            if (ui->settings.delay_ms > MIN_DELAY_MS)
            {
                if (ui->settings.delay_ms >= DELAY_STEP_MS)
                {
                    ui->settings.delay_ms -= DELAY_STEP_MS;
                }
                else
                {
                    ui->settings.delay_ms = MIN_DELAY_MS;
                }
                oled_display_update_delay(&ui->display, ui->settings.delay_ms);
                ui->settings_changed = true;
            }
            break;

        case EC11_PRESSED:
            // Enter sample rate menu
            ui->current_state = UI_STATE_MENU;
            ui->selected_sample_rate = ui_manager_get_sample_rate_option(ui->settings.sample_rate);
            oled_display_set_selection(&ui->display, ui->selected_sample_rate);
            oled_display_show_menu(&ui->display);
            break;

        default:
            break;
        }
        break;

    case UI_STATE_MENU:
        switch (event)
        {
        case EC11_CW:
            // Move selection down
            ui->selected_sample_rate = (ui->selected_sample_rate + 1) % SAMPLE_RATE_COUNT;
            oled_display_set_selection(&ui->display, ui->selected_sample_rate);
            break;

        case EC11_CCW:
            // Move selection up
            ui->selected_sample_rate = (ui->selected_sample_rate + SAMPLE_RATE_COUNT - 1) % SAMPLE_RATE_COUNT;
            oled_display_set_selection(&ui->display, ui->selected_sample_rate);
            break;

        case EC11_PRESSED:
            // Confirm selection
            ui->settings.sample_rate = ui_manager_get_sample_rate_value(ui->selected_sample_rate);
            oled_display_update_sample_rate(&ui->display, ui->settings.sample_rate);
            ui->display.menu_confirmed = true;
            ui->settings_changed = true;
            ui->current_state = UI_STATE_MENU_CONFIRM;
            break;

        default:
            break;
        }
        break;

    case UI_STATE_MENU_CONFIRM:
        switch (event)
        {
        case EC11_PRESSED:
            // Exit menu and return to main
            ui->current_state = UI_STATE_MAIN;
            ui->display.menu_confirmed = false;
            oled_display_show_main(&ui->display);
            break;

        default:
            // Any other event also exits menu
            ui->current_state = UI_STATE_MAIN;
            ui->display.menu_confirmed = false;
            oled_display_show_main(&ui->display);
            break;
        }
        break;
    }
}

esp_err_t ui_manager_update_display(ui_manager_t *ui)
{
    if (!ui)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Check for auto-save timeout
    uint64_t current_time = esp_timer_get_time() / 1000; // Convert to ms
    if (ui->settings_changed &&
        (current_time - ui->last_interaction_time) > AUTO_SAVE_TIMEOUT_MS)
    {
        ESP_ERROR_CHECK(ui_manager_save_settings(ui));
    }

    // Update display based on current state
    switch (ui->current_state)
    {
    case UI_STATE_MAIN:
        // Update main display if needed
        break;

    case UI_STATE_MENU:
        // Update menu display
        oled_display_show_menu(&ui->display);
        break;

    case UI_STATE_MENU_CONFIRM:
        // Update menu with confirmation
        oled_display_show_menu(&ui->display);
        break;
    }

    return ESP_OK;
}

esp_err_t ui_manager_save_settings(ui_manager_t *ui)
{
    if (!ui)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (ui->settings_changed)
    {
        ESP_ERROR_CHECK(settings_save(&ui->settings));
        ui->settings_changed = false;
        ESP_LOGI(TAG, "Settings auto-saved");
    }

    return ESP_OK;
}

uint32_t ui_manager_get_sample_rate_value(sample_rate_option_t option)
{
    if (option >= SAMPLE_RATE_COUNT)
    {
        return sample_rate_values[SAMPLE_RATE_48K]; // Default
    }
    return sample_rate_values[option];
}

sample_rate_option_t ui_manager_get_sample_rate_option(uint32_t sample_rate)
{
    for (int i = 0; i < SAMPLE_RATE_COUNT; i++)
    {
        if (sample_rate_values[i] == sample_rate)
        {
            return (sample_rate_option_t)i;
        }
    }
    return SAMPLE_RATE_48K; // Default
}

// Helper function to get current delay setting
uint32_t ui_manager_get_current_delay(ui_manager_t *ui)
{
    if (!ui)
    {
        return DEFAULT_DELAY_MS;
    }
    return ui->settings.delay_ms;
}

// Helper function to get current sample rate setting
uint32_t ui_manager_get_current_sample_rate(ui_manager_t *ui)
{
    if (!ui)
    {
        return DEFAULT_SAMPLE_RATE;
    }
    return ui->settings.sample_rate;
}

// Helper function to check if settings have changed
bool ui_manager_settings_changed(ui_manager_t *ui)
{
    if (!ui)
    {
        return false;
    }
    return ui->settings_changed;
}
