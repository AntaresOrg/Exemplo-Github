#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c.h"

uint16_t read_u16(const uint8_t *data);

int16_t read_i16(const uint8_t *data);

esp_err_t read_register(
    i2c_port_t i2c_port,
    uint8_t address,
    uint8_t reg,
    uint8_t *data,
    size_t length
);

esp_err_t write_register(
    i2c_port_t i2c_port,
    uint8_t address,
    uint8_t reg,
    uint8_t value
);

bool initialize_i2c(void);

bool is_i2c_initialized(void);
