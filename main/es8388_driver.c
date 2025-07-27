#include "es8388_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ES8388";

static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t es8388_handle = NULL;
static bool es8388_initialized = false;

// Internal function to write register
static esp_err_t es8388_write_reg(uint8_t reg, uint8_t data)
{
    if (!es8388_handle)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t write_buf[2] = {reg, data};
    esp_err_t ret = i2c_master_transmit(es8388_handle, write_buf, sizeof(write_buf), -1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write register 0x%02x: %s", reg, esp_err_to_name(ret));
    }
    return ret;
}

// Internal function to read register
static esp_err_t es8388_read_reg(uint8_t reg, uint8_t *data)
{
    if (!es8388_handle || !data)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = i2c_master_transmit_receive(es8388_handle, &reg, 1, data, 1, -1);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read register 0x%02x: %s", reg, esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t es8388_init(const es8388_config_t *config)
{
    if (!config)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (es8388_initialized)
    {
        ESP_LOGW(TAG, "ES8388 already initialized");
        return ESP_OK;
    }

    // Initialize I2C master bus
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = ES8388_I2C_SCL_PIN,
        .sda_io_num = ES8388_I2C_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Add ES8388 device to I2C bus
    i2c_device_config_t es8388_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ES8388_I2C_ADDR,
        .scl_speed_hz = ES8388_I2C_FREQ_HZ,
    };

    ret = i2c_master_bus_add_device(i2c_bus_handle, &es8388_cfg, &es8388_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add ES8388 device: %s", esp_err_to_name(ret));
        i2c_del_master_bus(i2c_bus_handle);
        return ret;
    }

    // Test communication with ES8388
    uint8_t chip_id;
    ret = es8388_read_reg(ES8388_CHIPPOWER, &chip_id);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to communicate with ES8388");
        i2c_master_bus_rm_device(es8388_handle);
        i2c_del_master_bus(i2c_bus_handle);
        return ret;
    }

    ESP_LOGI(TAG, "ES8388 detected, chip power register: 0x%02x", chip_id);

    // Reset ES8388
    ret = es8388_write_reg(ES8388_CONTROL1, 0x80);
    if (ret != ESP_OK)
    {
        goto init_fail;
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    ret = es8388_write_reg(ES8388_CONTROL1, 0x00);
    if (ret != ESP_OK)
    {
        goto init_fail;
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    // Power management
    ret = es8388_write_reg(ES8388_CHIPPOWER, 0x00); // Power up
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_MASTERMODE, 0x00); // Slave mode
    if (ret != ESP_OK)
        goto init_fail;

    // ADC configuration
    ret = es8388_write_reg(ES8388_ADCPOWER, 0x00); // Power up ADC
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_ADCCONTROL1, 0x88); // ADC L/R PGA gain
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_ADCCONTROL2, 0xF0); // ADC input select
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_ADCCONTROL3, 0x02); // ADC format
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_ADCCONTROL4, 0x0C); // ADC sample rate
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_ADCCONTROL5, 0x02); // ADC sample rate
    if (ret != ESP_OK)
        goto init_fail;

    // DAC configuration
    ret = es8388_write_reg(ES8388_DACPOWER, 0x00); // Power up DAC
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_DACCONTROL1, 0x18); // DAC format
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_DACCONTROL2, 0x02); // DAC sample rate
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_DACCONTROL3, 0x0C); // DAC sample rate
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_DACCONTROL4, 0x00); // DAC volume L
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_DACCONTROL5, 0x00); // DAC volume R
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_DACCONTROL6, 0x00); // DAC volume L
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_DACCONTROL7, 0x00); // DAC volume R
    if (ret != ESP_OK)
        goto init_fail;

    // Output configuration
    ret = es8388_write_reg(ES8388_DACCONTROL16, 0x00); // LOUT1 volume
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_DACCONTROL17, 0x90); // ROUT1 volume
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_DACCONTROL20, 0x90); // LOUT2 volume
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_DACCONTROL21, 0x90); // ROUT2 volume
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_DACCONTROL23, 0x00); // LOUT2 volume
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_DACCONTROL24, 0x00); // ROUT2 volume
    if (ret != ESP_OK)
        goto init_fail;

    // Enable outputs
    ret = es8388_write_reg(ES8388_DACCONTROL25, 0x50); // Enable LOUT1/ROUT1
    if (ret != ESP_OK)
        goto init_fail;

    ret = es8388_write_reg(ES8388_DACCONTROL26, 0x50); // Enable LOUT2/ROUT2
    if (ret != ESP_OK)
        goto init_fail;

    es8388_initialized = true;
    ESP_LOGI(TAG, "ES8388 initialized successfully");
    return ESP_OK;

init_fail:
    i2c_master_bus_rm_device(es8388_handle);
    i2c_del_master_bus(i2c_bus_handle);
    es8388_handle = NULL;
    i2c_bus_handle = NULL;
    return ret;
}

esp_err_t es8388_deinit(void)
{
    if (!es8388_initialized)
    {
        return ESP_OK;
    }

    // Power down ES8388
    es8388_write_reg(ES8388_CHIPPOWER, 0xFF);

    if (es8388_handle)
    {
        i2c_master_bus_rm_device(es8388_handle);
        es8388_handle = NULL;
    }

    if (i2c_bus_handle)
    {
        i2c_del_master_bus(i2c_bus_handle);
        i2c_bus_handle = NULL;
    }

    es8388_initialized = false;
    ESP_LOGI(TAG, "ES8388 deinitialized");
    return ESP_OK;
}

esp_err_t es8388_start(void)
{
    if (!es8388_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    // Start ADC and DAC
    esp_err_t ret = es8388_write_reg(ES8388_CHIPPOWER, 0x00);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ESP_LOGI(TAG, "ES8388 started");
    return ESP_OK;
}

esp_err_t es8388_stop(void)
{
    if (!es8388_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    // Stop ADC and DAC
    esp_err_t ret = es8388_write_reg(ES8388_CHIPPOWER, 0xFF);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ESP_LOGI(TAG, "ES8388 stopped");
    return ESP_OK;
}

esp_err_t es8388_set_volume(uint8_t volume)
{
    if (!es8388_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    // Volume range: 0-100, convert to ES8388 range (0-192)
    uint8_t es8388_vol = (volume * 192) / 100;

    esp_err_t ret = es8388_write_reg(ES8388_DACCONTROL4, es8388_vol);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = es8388_write_reg(ES8388_DACCONTROL5, es8388_vol);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ESP_LOGI(TAG, "Volume set to %d%%", volume);
    return ESP_OK;
}

esp_err_t es8388_mute(bool enable)
{
    if (!es8388_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t mute_val = enable ? 0x80 : 0x00;

    esp_err_t ret = es8388_write_reg(ES8388_DACCONTROL3, mute_val);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ESP_LOGI(TAG, "Mute %s", enable ? "enabled" : "disabled");
    return ESP_OK;
}

esp_err_t es8388_set_sample_rate(es8388_sample_rate_t sample_rate)
{
    if (!es8388_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t adc_rate_reg, dac_rate_reg;

    // Configure sample rate registers based on the sample rate
    switch (sample_rate)
    {
    case ES8388_SAMPLE_RATE_8K:
        adc_rate_reg = 0x1F;
        dac_rate_reg = 0x1F;
        break;
    case ES8388_SAMPLE_RATE_11K:
        adc_rate_reg = 0x17;
        dac_rate_reg = 0x17;
        break;
    case ES8388_SAMPLE_RATE_16K:
        adc_rate_reg = 0x0F;
        dac_rate_reg = 0x0F;
        break;
    case ES8388_SAMPLE_RATE_22K:
        adc_rate_reg = 0x0B;
        dac_rate_reg = 0x0B;
        break;
    case ES8388_SAMPLE_RATE_32K:
        adc_rate_reg = 0x07;
        dac_rate_reg = 0x07;
        break;
    case ES8388_SAMPLE_RATE_44K:
        adc_rate_reg = 0x05;
        dac_rate_reg = 0x05;
        break;
    case ES8388_SAMPLE_RATE_48K:
        adc_rate_reg = 0x04;
        dac_rate_reg = 0x04;
        break;
    case ES8388_SAMPLE_RATE_96K:
        adc_rate_reg = 0x02;
        dac_rate_reg = 0x02;
        break;
    case ES8388_SAMPLE_RATE_192K:
        adc_rate_reg = 0x01;
        dac_rate_reg = 0x01;
        break;
    default:
        ESP_LOGE(TAG, "Unsupported sample rate: %d", sample_rate);
        return ESP_ERR_INVALID_ARG;
    }

    // Set ADC sample rate
    esp_err_t ret = es8388_write_reg(ES8388_ADCCONTROL5, adc_rate_reg);
    if (ret != ESP_OK)
    {
        return ret;
    }

    // Set DAC sample rate
    ret = es8388_write_reg(ES8388_DACCONTROL2, dac_rate_reg);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ESP_LOGI(TAG, "Sample rate set to %d Hz", sample_rate);
    return ESP_OK;
}

esp_err_t es8388_set_bit_width(es8388_bit_width_t bit_width)
{
    if (!es8388_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t adc_format, dac_format;

    // Configure bit width registers
    switch (bit_width)
    {
    case ES8388_BIT_WIDTH_16:
        adc_format = 0x0C; // 16-bit I2S format
        dac_format = 0x18; // 16-bit I2S format
        break;
    case ES8388_BIT_WIDTH_18:
        adc_format = 0x04; // 18-bit I2S format
        dac_format = 0x10; // 18-bit I2S format
        break;
    case ES8388_BIT_WIDTH_20:
        adc_format = 0x08; // 20-bit I2S format
        dac_format = 0x14; // 20-bit I2S format
        break;
    case ES8388_BIT_WIDTH_24:
        adc_format = 0x00; // 24-bit I2S format
        dac_format = 0x1C; // 24-bit I2S format
        break;
    case ES8388_BIT_WIDTH_32:
        adc_format = 0x10; // 32-bit I2S format
        dac_format = 0x08; // 32-bit I2S format
        break;
    default:
        ESP_LOGE(TAG, "Unsupported bit width: %d", bit_width);
        return ESP_ERR_INVALID_ARG;
    }

    // Set ADC format
    esp_err_t ret = es8388_write_reg(ES8388_ADCCONTROL4, adc_format);
    if (ret != ESP_OK)
    {
        return ret;
    }

    // Set DAC format
    ret = es8388_write_reg(ES8388_DACCONTROL1, dac_format);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ESP_LOGI(TAG, "Bit width set to %d bits", bit_width);
    return ESP_OK;
}
