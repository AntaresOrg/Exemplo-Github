#include "i2c_utils.h"

#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c.h"
#include "pins.h"

static bool i2c_initialized = false;

bool initialize_i2c(void) {
    if (i2c_initialized) {
        return true;
    }

    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
        .clk_flags = 0
    };

    if (i2c_param_config(I2C_MASTER_NUM, &config) != ESP_OK) {
        return false;
    }

    if (i2c_driver_install(
            I2C_MASTER_NUM,
            I2C_MODE_MASTER,
            0,
            0,
            0) != ESP_OK) {
        return false;
    }

    i2c_initialized = true;
    return true;
}

bool is_i2c_initialized(void) {
    return i2c_initialized;
}

uint16_t read_u16(const uint8_t *data) {
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

int16_t read_i16(const uint8_t *data) {
    return (int16_t)data[0] | ((int16_t)data[1] << 8);
}

esp_err_t read_register(
    i2c_port_t i2c_port,
    uint8_t address,
    uint8_t reg,
    uint8_t *data,
    size_t length
) {
    return i2c_master_write_read_device(
        i2c_port,
        address,
        &reg,
        1,
        data,
        length,
        pdMS_TO_TICKS(100)
    );
}

esp_err_t write_register(
    i2c_port_t i2c_port,
    uint8_t address,
    uint8_t reg,
    uint8_t value
) {
    uint8_t buffer[2] = {reg, value};
    return i2c_master_write_read_device(
        i2c_port,
        address,
        buffer,
        sizeof(buffer),
        NULL,
        0,
        pdMS_TO_TICKS(100)
    );
}
