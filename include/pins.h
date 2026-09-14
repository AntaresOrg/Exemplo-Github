#pragma once

/**
 * This file contains the pin definitions that we will be using for the project.
 * You may change the pins to match you hardware setup, but make sure to update
 * the code accordingly.
 */
// I2C configuration
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_NUM I2C_NUM_0

// LED configuration
#define LED_GPIO GPIO_NUM_2