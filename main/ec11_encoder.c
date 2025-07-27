#include "ec11_encoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "EC11";

static ec11_callback_t g_callback = NULL;

// Encoder state lookup table for quadrature decoding
static const int8_t encoder_table[16] = {
    0,  // 0000
    1,  // 0001 (CW)
    -1, // 0010 (CCW)
    0,  // 0011
    -1, // 0100 (CCW)
    0,  // 0101
    0,  // 0110
    1,  // 0111 (CW)
    1,  // 1000 (CW)
    0,  // 1001
    0,  // 1010
    -1, // 1011 (CCW)
    0,  // 1100
    -1, // 1101 (CCW)
    1,  // 1110 (CW)
    0   // 1111
};

esp_err_t ec11_encoder_init(ec11_encoder_t *encoder, ec11_callback_t callback) {
    if (!encoder || !callback) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Initialize encoder structure
    encoder->pin_s1 = EC11_PIN_S1;
    encoder->pin_s2 = EC11_PIN_S2;
    encoder->pin_key = EC11_PIN_KEY;
    encoder->last_state = 0;
    encoder->key_pressed = false;
    encoder->last_key_time = 0;
    encoder->debounce_time_ms = 50;
    
    g_callback = callback;
    
    // Configure GPIO pins
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << encoder->pin_s1) | (1ULL << encoder->pin_s2),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    
    // Configure button pin
    io_conf.pin_bit_mask = (1ULL << encoder->pin_key);
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    
    // Read initial state
    encoder->last_state = (gpio_get_level(encoder->pin_s1) << 1) | gpio_get_level(encoder->pin_s2);
    
    ESP_LOGI(TAG, "EC11 encoder initialized");
    return ESP_OK;
}

esp_err_t ec11_encoder_deinit(ec11_encoder_t *encoder) {
    if (!encoder) {
        return ESP_ERR_INVALID_ARG;
    }
    
    g_callback = NULL;
    ESP_LOGI(TAG, "EC11 encoder deinitialized");
    return ESP_OK;
}

void ec11_encoder_task(void *pvParameters) {
    ec11_encoder_t *encoder = (ec11_encoder_t *)pvParameters;
    if (!encoder) {
        ESP_LOGE(TAG, "Invalid encoder parameter");
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "EC11 encoder task started");
    
    while (1) {
        // Read current encoder state
        uint8_t current_state = (gpio_get_level(encoder->pin_s1) << 1) | gpio_get_level(encoder->pin_s2);
        
        // Check for rotation
        if (current_state != encoder->last_state) {
            uint8_t combined_state = (encoder->last_state << 2) | current_state;
            int8_t direction = encoder_table[combined_state];
            
            if (direction != 0 && g_callback) {
                if (direction > 0) {
                    g_callback(EC11_CW);
                } else {
                    g_callback(EC11_CCW);
                }
            }
            
            encoder->last_state = current_state;
        }
        
        // Check button state
        bool key_current = !gpio_get_level(encoder->pin_key); // Active low
        uint64_t current_time = esp_timer_get_time() / 1000; // Convert to ms
        
        if (key_current != encoder->key_pressed) {
            if (current_time - encoder->last_key_time > encoder->debounce_time_ms) {
                encoder->key_pressed = key_current;
                encoder->last_key_time = current_time;
                
                if (g_callback) {
                    if (encoder->key_pressed) {
                        g_callback(EC11_PRESSED);
                    } else {
                        g_callback(EC11_RELEASED);
                    }
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(5)); // 5ms polling interval
    }
}
