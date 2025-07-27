#include "settings_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SETTINGS";
static nvs_handle_t nvs_handle_storage;
static bool nvs_initialized = false;

esp_err_t settings_manager_init(void)
{
    if (nvs_initialized)
    {
        return ESP_OK;
    }

    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle_storage);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(ret));
        return ret;
    }

    nvs_initialized = true;
    ESP_LOGI(TAG, "Settings manager initialized");
    return ESP_OK;
}

esp_err_t settings_manager_deinit(void)
{
    if (!nvs_initialized)
    {
        return ESP_OK;
    }

    nvs_close(nvs_handle_storage);
    nvs_initialized = false;
    ESP_LOGI(TAG, "Settings manager deinitialized");
    return ESP_OK;
}

esp_err_t settings_load(user_settings_t *settings)
{
    if (!settings)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!nvs_initialized)
    {
        ESP_LOGE(TAG, "Settings manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Load delay setting
    size_t required_size = sizeof(settings->delay_ms);
    esp_err_t ret = nvs_get_blob(nvs_handle_storage, NVS_KEY_DELAY_MS, &settings->delay_ms, &required_size);
    if (ret == ESP_ERR_NVS_NOT_FOUND)
    {
        settings->delay_ms = DEFAULT_DELAY_MS;
        ESP_LOGI(TAG, "Delay setting not found, using default: %d ms", DEFAULT_DELAY_MS);
    }
    else if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading delay setting: %s", esp_err_to_name(ret));
        settings->delay_ms = DEFAULT_DELAY_MS;
    }

    // Load sample rate setting
    required_size = sizeof(settings->sample_rate);
    ret = nvs_get_blob(nvs_handle_storage, NVS_KEY_SAMPLE_RATE, &settings->sample_rate, &required_size);
    if (ret == ESP_ERR_NVS_NOT_FOUND)
    {
        settings->sample_rate = DEFAULT_SAMPLE_RATE;
        ESP_LOGI(TAG, "Sample rate setting not found, using default: %d Hz", DEFAULT_SAMPLE_RATE);
    }
    else if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading sample rate setting: %s", esp_err_to_name(ret));
        settings->sample_rate = DEFAULT_SAMPLE_RATE;
    }

    ESP_LOGI(TAG, "Settings loaded - Delay: %d ms, Sample Rate: %d Hz",
             settings->delay_ms, settings->sample_rate);

    return ESP_OK;
}

esp_err_t settings_save(const user_settings_t *settings)
{
    if (!settings)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!nvs_initialized)
    {
        ESP_LOGE(TAG, "Settings manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Save delay setting
    esp_err_t ret = nvs_set_blob(nvs_handle_storage, NVS_KEY_DELAY_MS, &settings->delay_ms, sizeof(settings->delay_ms));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error saving delay setting: %s", esp_err_to_name(ret));
        return ret;
    }

    // Save sample rate setting
    ret = nvs_set_blob(nvs_handle_storage, NVS_KEY_SAMPLE_RATE, &settings->sample_rate, sizeof(settings->sample_rate));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error saving sample rate setting: %s", esp_err_to_name(ret));
        return ret;
    }

    // Commit changes
    ret = nvs_commit(nvs_handle_storage);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error committing settings: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Settings saved - Delay: %d ms, Sample Rate: %d Hz",
             settings->delay_ms, settings->sample_rate);

    return ESP_OK;
}

esp_err_t settings_reset_to_default(user_settings_t *settings)
{
    if (!settings)
    {
        return ESP_ERR_INVALID_ARG;
    }

    settings->delay_ms = DEFAULT_DELAY_MS;
    settings->sample_rate = DEFAULT_SAMPLE_RATE;

    ESP_LOGI(TAG, "Settings reset to default values");
    return ESP_OK;
}
