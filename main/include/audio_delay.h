#ifndef AUDIO_DELAY_H
#define AUDIO_DELAY_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2s_std.h"
#include "esp_err.h"

// Audio configuration constants
#define AUDIO_SAMPLE_RATE_44K 44100
#define AUDIO_SAMPLE_RATE_48K 48000
#define AUDIO_SAMPLE_RATE_96K 96000
#define AUDIO_SAMPLE_RATE_192K 192000

#define AUDIO_BITS_PER_SAMPLE 16
#define AUDIO_CHANNELS 1 // Mono

#define MIN_DELAY_MS 0
#define MAX_DELAY_MS 10000
#define DEFAULT_DELAY_MS 30
#define DELAY_STEP_MS 1

#define DEFAULT_SAMPLE_RATE AUDIO_SAMPLE_RATE_48K

// Audio buffer configuration
#define AUDIO_BUFFER_SIZE 1024
#define DELAY_BUFFER_SIZE (MAX_DELAY_MS * AUDIO_SAMPLE_RATE_192K / 1000 * 2) // Max buffer size

typedef struct
{
    uint32_t sample_rate;
    uint32_t delay_ms;
    int16_t *delay_buffer;
    uint32_t buffer_size;
    uint32_t write_index;
    uint32_t read_index;
    bool initialized;
} audio_delay_t;

// Function declarations
esp_err_t audio_delay_init(audio_delay_t *delay_ctx);
esp_err_t audio_delay_deinit(audio_delay_t *delay_ctx);
esp_err_t audio_delay_set_sample_rate(audio_delay_t *delay_ctx, uint32_t sample_rate);
esp_err_t audio_delay_set_delay(audio_delay_t *delay_ctx, uint32_t delay_ms);
esp_err_t audio_delay_process(audio_delay_t *delay_ctx, int16_t *input, int16_t *output, size_t samples);
void audio_delay_task(void *pvParameters);

#endif // AUDIO_DELAY_H
