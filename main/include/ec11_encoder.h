#ifndef EC11_ENCODER_H
#define EC11_ENCODER_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"

// EC11 encoder pin definitions for ESP32-A1S-AudioKit (基于可用扩展引脚)
#define EC11_PIN_S1 GPIO_NUM_22 // Encoder A (使用可插拔引脚)
#define EC11_PIN_S2 GPIO_NUM_19 // Encoder B (使用可插拔引脚)
#define EC11_PIN_KEY GPIO_NUM_0 // Push button (使用可插拔引脚)

// Encoder states
typedef enum
{
    EC11_IDLE,
    EC11_CW,      // Clockwise rotation
    EC11_CCW,     // Counter-clockwise rotation
    EC11_PRESSED, // Button pressed
    EC11_RELEASED // Button released
} ec11_event_t;

typedef struct
{
    gpio_num_t pin_s1;
    gpio_num_t pin_s2;
    gpio_num_t pin_key;
    uint8_t last_state;
    bool key_pressed;
    uint32_t last_key_time;
    uint32_t debounce_time_ms;
} ec11_encoder_t;

// Callback function type
typedef void (*ec11_callback_t)(ec11_event_t event);

// Function declarations
esp_err_t ec11_encoder_init(ec11_encoder_t *encoder, ec11_callback_t callback);
esp_err_t ec11_encoder_deinit(ec11_encoder_t *encoder);
void ec11_encoder_task(void *pvParameters);

#endif // EC11_ENCODER_H
