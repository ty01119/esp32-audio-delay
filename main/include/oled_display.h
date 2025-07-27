#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c.h"

// OLED display configuration
#define OLED_I2C_PORT I2C_NUM_0
#define OLED_I2C_ADDR 0x3C
#define OLED_SDA_PIN GPIO_NUM_21 // 使用独立I2C总线，避免与ES8388冲突
#define OLED_SCL_PIN GPIO_NUM_23
#define OLED_I2C_FREQ_HZ 400000

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES 8

// Display modes
typedef enum
{
    DISPLAY_MODE_MAIN, // Main delay display
    DISPLAY_MODE_MENU  // Sample rate menu
} display_mode_t;

typedef struct
{
    uint32_t current_delay_ms;
    uint32_t current_sample_rate;
    display_mode_t mode;
    uint8_t menu_selection;
    bool menu_confirmed;
} oled_display_t;

// Function declarations
esp_err_t oled_display_init(oled_display_t *display);
esp_err_t oled_display_deinit(oled_display_t *display);
esp_err_t oled_display_clear(void);
esp_err_t oled_display_update_delay(oled_display_t *display, uint32_t delay_ms);
esp_err_t oled_display_update_sample_rate(oled_display_t *display, uint32_t sample_rate);
esp_err_t oled_display_show_menu(oled_display_t *display);
esp_err_t oled_display_show_main(oled_display_t *display);
esp_err_t oled_display_set_selection(oled_display_t *display, uint8_t selection);

// Low-level display functions
esp_err_t oled_write_command(uint8_t cmd);
esp_err_t oled_write_data(uint8_t *data, size_t len);
esp_err_t oled_set_position(uint8_t page, uint8_t column);
esp_err_t oled_draw_string(uint8_t page, uint8_t column, const char *str, bool inverted);

#endif // OLED_DISPLAY_H
