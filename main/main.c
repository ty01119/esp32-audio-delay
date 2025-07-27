#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "audio_delay.h"
#include "ec11_encoder.h"
#include "oled_display.h"
#include "settings_manager.h"
#include "ui_manager.h"

static const char *TAG = "MAIN";

// Global variables
static audio_delay_t g_audio_delay;
static ec11_encoder_t g_encoder;
static ui_manager_t g_ui_manager;

// Task handles
static TaskHandle_t audio_task_handle = NULL;
static TaskHandle_t encoder_task_handle = NULL;

// Audio processing task
static void audio_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Audio task started");

    // Use the audio delay task implementation
    audio_delay_task(&g_audio_delay);
}

// Encoder event callback
static void encoder_callback(ec11_event_t event)
{
    ui_manager_handle_encoder_event(&g_ui_manager, event);
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Audio Delay starting...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize settings manager
    ESP_ERROR_CHECK(settings_manager_init());

    // Initialize UI manager
    ESP_ERROR_CHECK(ui_manager_init(&g_ui_manager));

    // Initialize audio delay
    ESP_ERROR_CHECK(audio_delay_init(&g_audio_delay));

    // Set initial audio delay parameters from UI settings
    ESP_ERROR_CHECK(audio_delay_set_delay(&g_audio_delay, ui_manager_get_current_delay(&g_ui_manager)));
    ESP_ERROR_CHECK(audio_delay_set_sample_rate(&g_audio_delay, ui_manager_get_current_sample_rate(&g_ui_manager)));

    // Initialize encoder
    ESP_ERROR_CHECK(ec11_encoder_init(&g_encoder, encoder_callback));

    // Create tasks
    xTaskCreate(audio_task, "audio_task", 4096, NULL, 5, &audio_task_handle);
    xTaskCreate(ec11_encoder_task, "encoder_task", 2048, &g_encoder, 4, &encoder_task_handle);

    ESP_LOGI(TAG, "System initialized successfully");

    // Main loop
    uint32_t last_delay = ui_manager_get_current_delay(&g_ui_manager);
    uint32_t last_sample_rate = ui_manager_get_current_sample_rate(&g_ui_manager);

    while (1)
    {
        // Update display
        ui_manager_update_display(&g_ui_manager);

        // Check for setting changes and update audio delay accordingly
        uint32_t current_delay = ui_manager_get_current_delay(&g_ui_manager);
        uint32_t current_sample_rate = ui_manager_get_current_sample_rate(&g_ui_manager);

        if (current_delay != last_delay)
        {
            ESP_ERROR_CHECK(audio_delay_set_delay(&g_audio_delay, current_delay));
            last_delay = current_delay;
            ESP_LOGI(TAG, "Audio delay updated to %d ms", current_delay);
        }

        if (current_sample_rate != last_sample_rate)
        {
            ESP_ERROR_CHECK(audio_delay_set_sample_rate(&g_audio_delay, current_sample_rate));
            last_sample_rate = current_sample_rate;
            ESP_LOGI(TAG, "Audio sample rate updated to %d Hz", current_sample_rate);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
