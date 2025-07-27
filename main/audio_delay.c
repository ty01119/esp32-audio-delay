#include "audio_delay.h"
#include "es8388_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

static const char *TAG = "AUDIO_DELAY";

// I2S pin configuration for ESP32-A1S-AudioKit (ES8388)
#define I2S_BCK_PIN GPIO_NUM_27
#define I2S_WS_PIN GPIO_NUM_25
#define I2S_DATA_IN_PIN GPIO_NUM_35
#define I2S_DATA_OUT_PIN GPIO_NUM_26

// I2S channel handles
static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;

esp_err_t audio_delay_init(audio_delay_t *delay_ctx)
{
    if (!delay_ctx)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize delay context
    delay_ctx->sample_rate = DEFAULT_SAMPLE_RATE;
    delay_ctx->delay_ms = DEFAULT_DELAY_MS;
    delay_ctx->buffer_size = DELAY_BUFFER_SIZE;
    delay_ctx->write_index = 0;
    delay_ctx->read_index = 0;
    delay_ctx->initialized = false;

    // Allocate delay buffer
    delay_ctx->delay_buffer = (int16_t *)malloc(DELAY_BUFFER_SIZE * sizeof(int16_t));
    if (!delay_ctx->delay_buffer)
    {
        ESP_LOGE(TAG, "Failed to allocate delay buffer");
        return ESP_ERR_NO_MEM;
    }

    // Clear delay buffer
    memset(delay_ctx->delay_buffer, 0, DELAY_BUFFER_SIZE * sizeof(int16_t));

    // Initialize ES8388 codec
    es8388_config_t es8388_cfg = ES8388_DEFAULT_CONFIG();
    es8388_cfg.sample_rate = (es8388_sample_rate_t)delay_ctx->sample_rate;

    esp_err_t ret = es8388_init(&es8388_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize ES8388: %s", esp_err_to_name(ret));
        free(delay_ctx->delay_buffer);
        return ret;
    }

    // Configure I2S channel
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 8;
    chan_cfg.dma_frame_num = AUDIO_BUFFER_SIZE;

    ret = i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s", esp_err_to_name(ret));
        es8388_deinit();
        free(delay_ctx->delay_buffer);
        return ret;
    }

    // Configure I2S standard mode
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(delay_ctx->sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_DATA_OUT_PIN,
            .din = I2S_DATA_IN_PIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(tx_handle, &std_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize I2S TX channel: %s", esp_err_to_name(ret));
        i2s_del_channel(tx_handle);
        i2s_del_channel(rx_handle);
        es8388_deinit();
        free(delay_ctx->delay_buffer);
        return ret;
    }

    ret = i2s_channel_init_std_mode(rx_handle, &std_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize I2S RX channel: %s", esp_err_to_name(ret));
        i2s_del_channel(tx_handle);
        i2s_del_channel(rx_handle);
        es8388_deinit();
        free(delay_ctx->delay_buffer);
        return ret;
    }

    // Enable I2S channels
    ret = i2s_channel_enable(tx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable I2S TX channel: %s", esp_err_to_name(ret));
        i2s_del_channel(tx_handle);
        i2s_del_channel(rx_handle);
        es8388_deinit();
        free(delay_ctx->delay_buffer);
        return ret;
    }

    ret = i2s_channel_enable(rx_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable I2S RX channel: %s", esp_err_to_name(ret));
        i2s_channel_disable(tx_handle);
        i2s_del_channel(tx_handle);
        i2s_del_channel(rx_handle);
        es8388_deinit();
        free(delay_ctx->delay_buffer);
        return ret;
    }

    // Start ES8388 codec
    ret = es8388_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start ES8388: %s", esp_err_to_name(ret));
        i2s_channel_disable(tx_handle);
        i2s_channel_disable(rx_handle);
        i2s_del_channel(tx_handle);
        i2s_del_channel(rx_handle);
        es8388_deinit();
        free(delay_ctx->delay_buffer);
        return ret;
    }

    delay_ctx->initialized = true;
    ESP_LOGI(TAG, "Audio delay initialized - Sample Rate: %" PRIu32 " Hz, Delay: %" PRIu32 " ms",
             delay_ctx->sample_rate, delay_ctx->delay_ms);

    return ESP_OK;
}

esp_err_t audio_delay_deinit(audio_delay_t *delay_ctx)
{
    if (!delay_ctx)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (delay_ctx->initialized)
    {
        // Stop ES8388 codec
        es8388_stop();

        if (tx_handle)
        {
            i2s_channel_disable(tx_handle);
            i2s_del_channel(tx_handle);
            tx_handle = NULL;
        }
        if (rx_handle)
        {
            i2s_channel_disable(rx_handle);
            i2s_del_channel(rx_handle);
            rx_handle = NULL;
        }

        // Deinitialize ES8388
        es8388_deinit();

        delay_ctx->initialized = false;
    }

    if (delay_ctx->delay_buffer)
    {
        free(delay_ctx->delay_buffer);
        delay_ctx->delay_buffer = NULL;
    }

    ESP_LOGI(TAG, "Audio delay deinitialized");
    return ESP_OK;
}

esp_err_t audio_delay_set_sample_rate(audio_delay_t *delay_ctx, uint32_t sample_rate)
{
    if (!delay_ctx)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Validate sample rate
    if (sample_rate != AUDIO_SAMPLE_RATE_44K &&
        sample_rate != AUDIO_SAMPLE_RATE_48K &&
        sample_rate != AUDIO_SAMPLE_RATE_96K &&
        sample_rate != AUDIO_SAMPLE_RATE_192K)
    {
        ESP_LOGE(TAG, "Unsupported sample rate: %" PRIu32, sample_rate);
        return ESP_ERR_INVALID_ARG;
    }

    delay_ctx->sample_rate = sample_rate;

    // Reconfigure I2S if initialized
    if (delay_ctx->initialized && tx_handle && rx_handle)
    {
        // For the new I2S driver, we need to reconfigure the clock
        i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate);

        esp_err_t ret = i2s_channel_reconfig_std_clock(tx_handle, &clk_cfg);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set I2S TX sample rate: %s", esp_err_to_name(ret));
            return ret;
        }

        ret = i2s_channel_reconfig_std_clock(rx_handle, &clk_cfg);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set I2S RX sample rate: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    // Recalculate read index based on new sample rate
    uint32_t delay_samples = (delay_ctx->delay_ms * sample_rate) / 1000;
    delay_ctx->read_index = (delay_ctx->write_index + delay_ctx->buffer_size - delay_samples) % delay_ctx->buffer_size;

    ESP_LOGI(TAG, "Sample rate changed to %" PRIu32 " Hz", sample_rate);
    return ESP_OK;
}

esp_err_t audio_delay_set_delay(audio_delay_t *delay_ctx, uint32_t delay_ms)
{
    if (!delay_ctx)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Validate delay range
    if (delay_ms > MAX_DELAY_MS)
    {
        ESP_LOGE(TAG, "Delay out of range: %" PRIu32 " ms (valid range: %d-%d ms)",
                 delay_ms, MIN_DELAY_MS, MAX_DELAY_MS);
        return ESP_ERR_INVALID_ARG;
    }

    delay_ctx->delay_ms = delay_ms;

    // Recalculate read index based on new delay
    uint32_t delay_samples = (delay_ms * delay_ctx->sample_rate) / 1000;

    // Ensure delay doesn't exceed buffer size
    if (delay_samples >= delay_ctx->buffer_size)
    {
        ESP_LOGE(TAG, "Delay too large for buffer: %" PRIu32 " samples (max: %" PRIu32 ")",
                 delay_samples, delay_ctx->buffer_size - 1);
        return ESP_ERR_INVALID_ARG;
    }

    delay_ctx->read_index = (delay_ctx->write_index + delay_ctx->buffer_size - delay_samples) % delay_ctx->buffer_size;

    ESP_LOGI(TAG, "Delay changed to %" PRIu32 " ms (%" PRIu32 " samples)", delay_ms, delay_samples);
    return ESP_OK;
}

esp_err_t audio_delay_process(audio_delay_t *delay_ctx, int16_t *input, int16_t *output, size_t samples)
{
    if (!delay_ctx || !input || !output)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (!delay_ctx->initialized)
    {
        ESP_LOGE(TAG, "Audio delay not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    for (size_t i = 0; i < samples; i++)
    {
        // Write input sample to delay buffer
        delay_ctx->delay_buffer[delay_ctx->write_index] = input[i];

        // Read delayed sample from buffer
        output[i] = delay_ctx->delay_buffer[delay_ctx->read_index];

        // Advance indices with wrap-around
        delay_ctx->write_index = (delay_ctx->write_index + 1) % delay_ctx->buffer_size;
        delay_ctx->read_index = (delay_ctx->read_index + 1) % delay_ctx->buffer_size;
    }

    return ESP_OK;
}

// Audio processing task function (to be called from main)
void audio_delay_task(void *pvParameters)
{
    audio_delay_t *delay_ctx = (audio_delay_t *)pvParameters;
    if (!delay_ctx)
    {
        ESP_LOGE(TAG, "Invalid delay context parameter");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Audio delay task started");

    int16_t *input_buffer = malloc(AUDIO_BUFFER_SIZE * sizeof(int16_t));
    int16_t *output_buffer = malloc(AUDIO_BUFFER_SIZE * sizeof(int16_t));

    if (!input_buffer || !output_buffer)
    {
        ESP_LOGE(TAG, "Failed to allocate audio buffers");
        if (input_buffer)
            free(input_buffer);
        if (output_buffer)
            free(output_buffer);
        vTaskDelete(NULL);
        return;
    }

    size_t bytes_read, bytes_written;

    while (1)
    {
        // Read audio data from I2S
        esp_err_t ret = i2s_channel_read(rx_handle, input_buffer, AUDIO_BUFFER_SIZE * sizeof(int16_t),
                                         &bytes_read, portMAX_DELAY);

        if (ret == ESP_OK && bytes_read > 0)
        {
            size_t samples_read = bytes_read / sizeof(int16_t);

            // Process audio through delay
            ret = audio_delay_process(delay_ctx, input_buffer, output_buffer, samples_read);

            if (ret == ESP_OK)
            {
                // Write processed audio to I2S
                ret = i2s_channel_write(tx_handle, output_buffer, samples_read * sizeof(int16_t),
                                        &bytes_written, portMAX_DELAY);

                if (ret != ESP_OK)
                {
                    ESP_LOGE(TAG, "I2S write error: %s", esp_err_to_name(ret));
                }
            }
            else
            {
                ESP_LOGE(TAG, "Audio delay processing error: %s", esp_err_to_name(ret));
            }
        }
        else
        {
            ESP_LOGE(TAG, "I2S read error: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    free(input_buffer);
    free(output_buffer);
    vTaskDelete(NULL);
}
