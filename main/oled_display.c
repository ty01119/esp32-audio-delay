#include "oled_display.h"
#include "audio_delay.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

static const char *TAG = "OLED";

// SSD1306 commands
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_SEGREMAP 0xA1
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR 0x22

// 8x8 font data (simplified ASCII characters 0-9, A-Z, space, colon, dot)
static const uint8_t font_8x8[][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ' ' (32)
    {0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00}, // '0' (48)
    {0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00}, // '1'
    {0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00}, // '2'
    {0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00}, // '3'
    {0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00}, // '4'
    {0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00}, // '5'
    {0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00}, // '6'
    {0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00}, // '7'
    {0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00}, // '8'
    {0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00}, // '9'
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // ':' (58)
    {0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00}, // 'A' (65)
    {0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00}, // 'B'
    {0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00}, // 'C'
    {0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00}, // 'D'
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00}, // 'E'
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00}, // 'F'
    {0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00}, // 'G'
    {0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00}, // 'H'
    {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // 'I'
    {0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00}, // 'J'
    {0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00}, // 'K'
    {0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00}, // 'L'
    {0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00}, // 'M'
    {0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00}, // 'N'
    {0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00}, // 'O'
    {0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00}, // 'P'
    {0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00}, // 'Q'
    {0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00}, // 'R'
    {0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00}, // 'S'
    {0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // 'T'
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00}, // 'U'
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // 'V'
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // 'W'
    {0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00}, // 'X'
    {0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00}, // 'Y'
    {0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00}, // 'Z'
};

static bool i2c_initialized = false;

static esp_err_t i2c_master_init(void)
{
    if (i2c_initialized)
    {
        return ESP_OK;
    }

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = OLED_SDA_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = OLED_SCL_PIN,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = OLED_I2C_FREQ_HZ,
    };

    esp_err_t ret = i2c_param_config(OLED_I2C_PORT, &conf);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = i2c_driver_install(OLED_I2C_PORT, conf.mode, 0, 0, 0);
    if (ret != ESP_OK)
    {
        return ret;
    }

    i2c_initialized = true;
    return ESP_OK;
}

esp_err_t oled_write_command(uint8_t cmd)
{
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (OLED_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, 0x00, true); // Command mode
    i2c_master_write_byte(cmd_handle, cmd, true);
    i2c_master_stop(cmd_handle);
    esp_err_t ret = i2c_master_cmd_begin(OLED_I2C_PORT, cmd_handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd_handle);
    return ret;
}

esp_err_t oled_write_data(uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (OLED_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, 0x40, true); // Data mode
    i2c_master_write(cmd_handle, data, len, true);
    i2c_master_stop(cmd_handle);
    esp_err_t ret = i2c_master_cmd_begin(OLED_I2C_PORT, cmd_handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd_handle);
    return ret;
}

static uint8_t get_font_index(char c)
{
    if (c == ' ')
        return 0;
    if (c >= '0' && c <= '9')
        return c - '0' + 1;
    if (c == ':')
        return 11;
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 12;
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 12; // Convert lowercase to uppercase
    return 0;                // Default to space
}

esp_err_t oled_display_init(oled_display_t *display)
{
    if (!display)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize I2C
    ESP_ERROR_CHECK(i2c_master_init());

    // Initialize display structure
    display->current_delay_ms = DEFAULT_DELAY_MS;
    display->current_sample_rate = DEFAULT_SAMPLE_RATE;
    display->mode = DISPLAY_MODE_MAIN;
    display->menu_selection = 0;
    display->menu_confirmed = false;

    // SSD1306 initialization sequence
    ESP_ERROR_CHECK(oled_write_command(SSD1306_DISPLAYOFF));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_SETDISPLAYCLOCKDIV));
    ESP_ERROR_CHECK(oled_write_command(0x80));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_SETMULTIPLEX));
    ESP_ERROR_CHECK(oled_write_command(0x3F));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_SETDISPLAYOFFSET));
    ESP_ERROR_CHECK(oled_write_command(0x00));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_SETSTARTLINE | 0x00));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_CHARGEPUMP));
    ESP_ERROR_CHECK(oled_write_command(0x14));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_MEMORYMODE));
    ESP_ERROR_CHECK(oled_write_command(0x00));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_SEGREMAP | 0x01));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_COMSCANDEC));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_SETCOMPINS));
    ESP_ERROR_CHECK(oled_write_command(0x12));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_SETCONTRAST));
    ESP_ERROR_CHECK(oled_write_command(0xCF));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_SETPRECHARGE));
    ESP_ERROR_CHECK(oled_write_command(0xF1));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_SETVCOMDETECT));
    ESP_ERROR_CHECK(oled_write_command(0x40));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_NORMALDISPLAY));
    ESP_ERROR_CHECK(oled_write_command(SSD1306_DISPLAYON));

    // Clear display
    ESP_ERROR_CHECK(oled_display_clear());

    ESP_LOGI(TAG, "OLED display initialized");
    return ESP_OK;
}

esp_err_t oled_display_deinit(oled_display_t *display)
{
    if (!display)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_ERROR_CHECK(oled_write_command(SSD1306_DISPLAYOFF));

    if (i2c_initialized)
    {
        i2c_driver_delete(OLED_I2C_PORT);
        i2c_initialized = false;
    }

    ESP_LOGI(TAG, "OLED display deinitialized");
    return ESP_OK;
}

esp_err_t oled_display_clear(void)
{
    // Set column address range
    ESP_ERROR_CHECK(oled_write_command(SSD1306_COLUMNADDR));
    ESP_ERROR_CHECK(oled_write_command(0));
    ESP_ERROR_CHECK(oled_write_command(OLED_WIDTH - 1));

    // Set page address range
    ESP_ERROR_CHECK(oled_write_command(SSD1306_PAGEADDR));
    ESP_ERROR_CHECK(oled_write_command(0));
    ESP_ERROR_CHECK(oled_write_command(OLED_PAGES - 1));

    // Clear all pixels
    uint8_t clear_data[128];
    memset(clear_data, 0x00, sizeof(clear_data));

    for (int page = 0; page < OLED_PAGES; page++)
    {
        ESP_ERROR_CHECK(oled_write_data(clear_data, sizeof(clear_data)));
    }

    return ESP_OK;
}

esp_err_t oled_set_position(uint8_t page, uint8_t column)
{
    ESP_ERROR_CHECK(oled_write_command(SSD1306_COLUMNADDR));
    ESP_ERROR_CHECK(oled_write_command(column));
    ESP_ERROR_CHECK(oled_write_command(OLED_WIDTH - 1));

    ESP_ERROR_CHECK(oled_write_command(SSD1306_PAGEADDR));
    ESP_ERROR_CHECK(oled_write_command(page));
    ESP_ERROR_CHECK(oled_write_command(page));

    return ESP_OK;
}

esp_err_t oled_draw_string(uint8_t page, uint8_t column, const char *str, bool inverted)
{
    if (!str)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_ERROR_CHECK(oled_set_position(page, column));

    uint8_t char_data[8];
    size_t len = strlen(str);

    for (size_t i = 0; i < len && column < OLED_WIDTH; i++)
    {
        uint8_t font_idx = get_font_index(str[i]);

        // Copy font data and apply inversion if needed
        for (int j = 0; j < 8; j++)
        {
            char_data[j] = inverted ? ~font_8x8[font_idx][j] : font_8x8[font_idx][j];
        }

        ESP_ERROR_CHECK(oled_write_data(char_data, 8));
        column += 8;
    }

    return ESP_OK;
}

esp_err_t oled_display_update_delay(oled_display_t *display, uint32_t delay_ms)
{
    if (!display)
    {
        return ESP_ERR_INVALID_ARG;
    }

    display->current_delay_ms = delay_ms;
    return ESP_OK;
}

esp_err_t oled_display_update_sample_rate(oled_display_t *display, uint32_t sample_rate)
{
    if (!display)
    {
        return ESP_ERR_INVALID_ARG;
    }

    display->current_sample_rate = sample_rate;
    return ESP_OK;
}

esp_err_t oled_display_show_main(oled_display_t *display)
{
    if (!display)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_ERROR_CHECK(oled_display_clear());

    // Display title
    ESP_ERROR_CHECK(oled_draw_string(0, 16, "AUDIO DELAY", false));

    // Display current delay
    char delay_str[32];
    snprintf(delay_str, sizeof(delay_str), "DELAY: %" PRIu32 "MS", display->current_delay_ms);
    ESP_ERROR_CHECK(oled_draw_string(3, 8, delay_str, false));

    // Display current sample rate
    char rate_str[32];
    if (display->current_sample_rate >= 1000)
    {
        snprintf(rate_str, sizeof(rate_str), "RATE: %" PRIu32 "KHZ", display->current_sample_rate / 1000);
    }
    else
    {
        snprintf(rate_str, sizeof(rate_str), "RATE: %" PRIu32 "HZ", display->current_sample_rate);
    }
    ESP_ERROR_CHECK(oled_draw_string(5, 8, rate_str, false));

    display->mode = DISPLAY_MODE_MAIN;
    return ESP_OK;
}

esp_err_t oled_display_show_menu(oled_display_t *display)
{
    if (!display)
    {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_ERROR_CHECK(oled_display_clear());

    // Display menu title
    ESP_ERROR_CHECK(oled_draw_string(0, 16, "SAMPLE RATE", false));

    // Sample rate options
    const char *rate_options[] = {"44.1KHZ", "48KHZ", "96KHZ", "192KHZ"};
    const uint32_t rate_values[] = {44100, 48000, 96000, 192000};

    for (int i = 0; i < 4; i++)
    {
        bool selected = (i == display->menu_selection);
        bool confirmed = (rate_values[i] == display->current_sample_rate) && display->menu_confirmed;

        // Draw selection indicator
        if (selected)
        {
            ESP_ERROR_CHECK(oled_draw_string(i + 2, 0, ">", false));
        }

        // Draw option text (inverted if selected)
        ESP_ERROR_CHECK(oled_draw_string(i + 2, 16, rate_options[i], selected));

        // Draw confirmation indicator (underline)
        if (confirmed)
        {
            ESP_ERROR_CHECK(oled_draw_string(i + 2, 80, "_", false));
        }
    }

    display->mode = DISPLAY_MODE_MENU;
    return ESP_OK;
}

esp_err_t oled_display_set_selection(oled_display_t *display, uint8_t selection)
{
    if (!display)
    {
        return ESP_ERR_INVALID_ARG;
    }

    display->menu_selection = selection;
    return ESP_OK;
}
