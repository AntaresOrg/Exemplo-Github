#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * In this file we want you to implement the interface for the BMP280 sensor.
 * The BMP280 is a barometric pressure sensor that can be used to measure atmospheric
 * pressure and temperature, which can then be used to calculate altitude. The 
 * interface should provide functions to initialize the sensor, read data from it,
 * and check its status.
 * 
 * For this assignment, you will need to implement the following functions in a .c file:
 * 
 * - bmp280_init: This function should initialize the BMP280 sensor with the given I2C
 *   address and name. It should return a pointer to a bmp280_t structure that represents
 *   the sensor.
 * 
 * - bmp280_is_initialized: This function should return true if the BMP280 sensor has been
 *   successfully initialized, and false otherwise.
 * 
 * - bmp280_get_absolute_altitude: This function should read the current pressure and 
 *   temperature from the BMP280 sensor, and use these values to calculate the absolute
 *   altitude.
 * 
 * - bmp280_get_relative_altitude: This function should read the current pressure and
 *   temperature from the BMP280 sensor, and use these values to calculate the relative
 *   altitude from a reference point calculated when the sensor is initialized.
 * 
 * You may also want to implement additional helper functions to read raw data from the sensor,
 * convert it to meaningful values, and perform any necessary calculations. Be sure to
 * handle any errors that may occur during communication with the sensor, and return appropriate
 * error codes or status values from your functions.
 * 
 * Also remember to at least document these functions in the header file, so that other developers
 * can understand how to use your interface.
 */
typedef struct bmp280 bmp280_t;

// BMP280 ADDRESSES
#define BMP280_ADDR_1 0x76
#define BMP280_ADDR_2 0x77

bool bmp280_is_initialized(const bmp280_t *bmp);

bmp280_t *bmp280_init(uint8_t address, const char *name);

float bmp280_get_relative_altitude(bmp280_t *bmp);

float bmp280_get_absolute_altitude(bmp280_t *bmp);
