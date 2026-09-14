#include "bmp280.h"
#include "configs.h"
#include "pins.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

    // Initialize BMP280 sensors
    bmp280_t *bmp1 = bmp280_init(BMP280_ADDR_1, "BMP1");
    bmp280_t *bmp2 = bmp280_init(BMP280_ADDR_2, "BMP2");

    if (!bmp1 || !bmp2) {
        // Initialization failed
        gpio_set_level(LED_GPIO, 0);
        return;
    }

    // Main loop
    while (1) {
        float altitude1;
        float altitude2;

        // Read relative altitude from both sensors, the valid1 and valid2
        // variables will be true if the readings were successful
        bool valid1 = bmp280_get_relative_altitude(bmp1, &altitude1);
        bool valid2 = bmp280_get_relative_altitude(bmp2, &altitude2);

        if (valid1 && valid2) {
            if ((altitude1 + altitude2) / 2.0f > ALTITUDE_THRESHOLD_M) {
                gpio_set_level(LED_GPIO, 1);
            } else {
                gpio_set_level(LED_GPIO, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
