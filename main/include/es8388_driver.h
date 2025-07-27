#ifndef ES8388_DRIVER_H
#define ES8388_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

// ES8388 I2C configuration
#define ES8388_I2C_ADDR 0x10
#define ES8388_I2C_SDA_PIN GPIO_NUM_33
#define ES8388_I2C_SCL_PIN GPIO_NUM_32
#define ES8388_I2C_FREQ_HZ 100000

// ES8388 Register definitions
#define ES8388_CONTROL1         0x00
#define ES8388_CONTROL2         0x01
#define ES8388_CHIPPOWER        0x02
#define ES8388_ADCPOWER         0x03
#define ES8388_DACPOWER         0x04
#define ES8388_CHIPLOPOW1       0x05
#define ES8388_CHIPLOPOW2       0x06
#define ES8388_ANAVOLMANAG      0x07
#define ES8388_MASTERMODE       0x08
#define ES8388_ADCCONTROL1      0x09
#define ES8388_ADCCONTROL2      0x0A
#define ES8388_ADCCONTROL3      0x0B
#define ES8388_ADCCONTROL4      0x0C
#define ES8388_ADCCONTROL5      0x0D
#define ES8388_ADCCONTROL6      0x0E
#define ES8388_ADCCONTROL7      0x0F
#define ES8388_ADCCONTROL8      0x10
#define ES8388_ADCCONTROL9      0x11
#define ES8388_ADCCONTROL10     0x12
#define ES8388_ADCCONTROL11     0x13
#define ES8388_ADCCONTROL12     0x14
#define ES8388_ADCCONTROL13     0x15
#define ES8388_ADCCONTROL14     0x16
#define ES8388_DACCONTROL1      0x17
#define ES8388_DACCONTROL2      0x18
#define ES8388_DACCONTROL3      0x19
#define ES8388_DACCONTROL4      0x1A
#define ES8388_DACCONTROL5      0x1B
#define ES8388_DACCONTROL6      0x1C
#define ES8388_DACCONTROL7      0x1D
#define ES8388_DACCONTROL8      0x1E
#define ES8388_DACCONTROL9      0x1F
#define ES8388_DACCONTROL10     0x20
#define ES8388_DACCONTROL11     0x21
#define ES8388_DACCONTROL12     0x22
#define ES8388_DACCONTROL13     0x23
#define ES8388_DACCONTROL14     0x24
#define ES8388_DACCONTROL15     0x25
#define ES8388_DACCONTROL16     0x26
#define ES8388_DACCONTROL17     0x27
#define ES8388_DACCONTROL18     0x28
#define ES8388_DACCONTROL19     0x29
#define ES8388_DACCONTROL20     0x2A
#define ES8388_DACCONTROL21     0x2B
#define ES8388_DACCONTROL22     0x2C
#define ES8388_DACCONTROL23     0x2D
#define ES8388_DACCONTROL24     0x2E
#define ES8388_DACCONTROL25     0x2F
#define ES8388_DACCONTROL26     0x30
#define ES8388_DACCONTROL27     0x31
#define ES8388_DACCONTROL28     0x32
#define ES8388_DACCONTROL29     0x33
#define ES8388_DACCONTROL30     0x34

// ES8388 sample rate definitions
typedef enum {
    ES8388_SAMPLE_RATE_8K = 8000,
    ES8388_SAMPLE_RATE_11K = 11025,
    ES8388_SAMPLE_RATE_16K = 16000,
    ES8388_SAMPLE_RATE_22K = 22050,
    ES8388_SAMPLE_RATE_32K = 32000,
    ES8388_SAMPLE_RATE_44K = 44100,
    ES8388_SAMPLE_RATE_48K = 48000,
    ES8388_SAMPLE_RATE_96K = 96000,
    ES8388_SAMPLE_RATE_192K = 192000
} es8388_sample_rate_t;

// ES8388 bit width definitions
typedef enum {
    ES8388_BIT_WIDTH_16 = 16,
    ES8388_BIT_WIDTH_18 = 18,
    ES8388_BIT_WIDTH_20 = 20,
    ES8388_BIT_WIDTH_24 = 24,
    ES8388_BIT_WIDTH_32 = 32
} es8388_bit_width_t;

// ES8388 configuration structure
typedef struct {
    es8388_sample_rate_t sample_rate;
    es8388_bit_width_t bit_width;
    uint8_t adc_input;      // ADC input selection
    uint8_t dac_output;     // DAC output selection
    bool mic_gain;          // Microphone gain enable
} es8388_config_t;

// Function declarations
esp_err_t es8388_init(const es8388_config_t *config);
esp_err_t es8388_deinit(void);
esp_err_t es8388_set_sample_rate(es8388_sample_rate_t sample_rate);
esp_err_t es8388_set_bit_width(es8388_bit_width_t bit_width);
esp_err_t es8388_set_volume(uint8_t volume);
esp_err_t es8388_mute(bool enable);
esp_err_t es8388_start(void);
esp_err_t es8388_stop(void);

// Default configuration
#define ES8388_DEFAULT_CONFIG() { \
    .sample_rate = ES8388_SAMPLE_RATE_48K, \
    .bit_width = ES8388_BIT_WIDTH_16, \
    .adc_input = 0x00, \
    .dac_output = 0x3C, \
    .mic_gain = false \
}

#endif // ES8388_DRIVER_H
