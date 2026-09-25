#include "bmp280.h"
#include "configs.h"
#include "pins.h"
#include "i2c_utils.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <math.h>

#define TAG "MAIN"

/**
 * Main application entry point.
 * This is where the code will execute after the system has been initialized.
 * In this function, we will initialize the BMP280 sensors and the LED, and 
 * then enter a loop where we read the altitude from both sensors and control
 * the LED based on the average altitude.
 */
void app_main(void)
{
    // Initialize LED
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    // Configure the LED GPIO
    gpio_config(&led_config);
    gpio_set_level(LED_GPIO, 0);

    // Initialize I2C
    if (!initialize_i2c()) {
        // I2C initialization failed
        gpio_set_level(LED_GPIO, 0);
        return;
    }

    // Initialize BMP280 sensors
    bmp280_t *bmp1 = bmp280_init(BMP280_ADDR_1, "BMP1");
    bmp280_t *bmp2 = bmp280_init(BMP280_ADDR_2, "BMP2");

    ESP_LOGI(TAG, "BMP1 initialization: %s", bmp280_is_initialized(bmp1) ? "SUCCESS" : "FAILED");
    ESP_LOGI(TAG, "BMP2 initialization: %s", bmp280_is_initialized(bmp2) ? "SUCCESS" : "FAILED");

    if (!bmp1 || !bmp2) {
        // Initialization failed
        gpio_set_level(LED_GPIO, 0);
        return;
    }

    // Main loop
    while (1) {
        // Read relative altitude from both sensors, the valid1 and valid2
        // variables will be true if the readings were successful
        float altitude1 = bmp280_get_relative_altitude(bmp1);
        float altitude2 = bmp280_get_relative_altitude(bmp2);

        ESP_LOGI(TAG, "Current relative altitude: BMP1=%.2f m", altitude1);
        ESP_LOGI(TAG, "Current relative altitude: BMP2=%.2f m", altitude2);
        ESP_LOGI(TAG, "Average relative altitude: %.2f m", (altitude1 + altitude2) / 2.0f);

        if (!isfinite(altitude1) || !isfinite(altitude2)) {
            gpio_set_level(LED_GPIO, 0);
        } else if ((altitude1 + altitude2) / 2.0f > ALTITUDE_THRESHOLD_M) {
            gpio_set_level(LED_GPIO, 1);
        } else {
            gpio_set_level(LED_GPIO, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}