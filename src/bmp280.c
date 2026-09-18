#include "bmp280.h"

// Standard and project dependencies.
#include <math.h>
#include <stdlib.h>
#include "driver/i2c.h"
#include "pins.h"
#include "i2c_utils.h"

struct bmp280 {
    // Sensor identity and initialization state.
    uint8_t address;
    const char *name;
    bool initialized;
    float reference_altitude_m;

    // Temperature calibration coefficients read from registers 0x88-0x8D.
    uint16_t dig_t1;
    int16_t dig_t2;
    int16_t dig_t3;

    // Pressure calibration coefficients read from registers 0x8E-0x9F.
    uint16_t dig_p1;
    int16_t dig_p2;
    int16_t dig_p3;
    int16_t dig_p4;
    int16_t dig_p5;
    int16_t dig_p6;
    int16_t dig_p7;
    int16_t dig_p8;
    int16_t dig_p9;
};


#define BMP280_SEA_LEVEL_PA 101325.0f
#define BMP280_BASELINE_STABILIZATION_SAMPLES 10

static int32_t get_t_fine(const bmp280_t *bmp, const uint8_t *data)
{
    if (!bmp || !data) {
        return 0;
    }

    // The temperature ADC value occupies bytes 3-5 of the sensor data frame.
    int32_t adc_t = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | ((int32_t)(data[5] >> 4));

    // t_fine is an intermediate value used by both temperature and pressure compensation.
    int32_t var1 = ((((adc_t >> 3) - ((int32_t)bmp->dig_t1 << 1))) * ((int32_t)bmp->dig_t2)) >> 11;
    int32_t var2 = (((((adc_t >> 4) - ((int32_t)bmp->dig_t1)) * ((adc_t >> 4) - ((int32_t)bmp->dig_t1))) >> 12) * ((int32_t)bmp->dig_t3)) >> 14;
    int32_t t_fine = var1 + var2;

    return t_fine;
}

float get_pressure(bmp280_t *bmp)
{
    uint8_t data[6];

    // Read pressure and temperature together so both values belong to the same sample.
    if (!bmp || !bmp->initialized ||
        read_register(I2C_MASTER_NUM, bmp->address, 0xF7, data, sizeof(data)) != ESP_OK) {
        return -1.0f;
    }

    int32_t adc_p = ((int32_t)data[0] << 12) |
                    ((int32_t)data[1] << 4) |
                    (data[2] >> 4);
    int32_t t_fine = get_t_fine(bmp, data);

    // Apply the BMP280 pressure compensation formula from the datasheet.
    int64_t var1_p = (int64_t)t_fine - 128000;
    int64_t var2_p = var1_p * var1_p * (int64_t)bmp->dig_p6;
    var2_p += (var1_p * (int64_t)bmp->dig_p5) << 17;
    var2_p += ((int64_t)bmp->dig_p4) << 35;
    var1_p = ((var1_p * var1_p * (int64_t)bmp->dig_p3) >> 8) + ((var1_p * (int64_t)bmp->dig_p2) << 12);
    var1_p = (((((int64_t)1) << 47) + var1_p)) * ((int64_t)bmp->dig_p1) >> 33;

    if (var1_p == 0) {
        return -1.0f;
    }

    int64_t pressure = 1048576 - adc_p;
    pressure = (((pressure << 31) - var2_p) * 3125) / var1_p;
    var1_p = ((int64_t)bmp->dig_p9 * (pressure >> 13) *
              (pressure >> 13)) >> 25;
    var2_p = ((int64_t)bmp->dig_p8 * pressure) >> 19;
    pressure = ((pressure + var1_p + var2_p) >> 8) +
               ((int64_t)bmp->dig_p7 << 4);

    float pressure_pa = (float)pressure / 256.0f;
    return isfinite(pressure_pa) && pressure_pa > 0.0f ? pressure_pa : -1.0f;
}

static float bmp280_pressure_to_altitude(float pressure_pa, float sea_level_pa)
{
    if (!isfinite(pressure_pa) || pressure_pa <= 0.0f ||
        !isfinite(sea_level_pa) || sea_level_pa <= 0.0f) {
        return NAN;
    }

    // Convert pressure relative to sea-level pressure into altitude in meters.
    return 44330.0f * (1.0f - powf(pressure_pa / sea_level_pa, 0.1903f));
}

bmp280_t *bmp280_init(uint8_t address, const char *name)
{
    // The I2C bus must be configured before a sensor can be probed.
    if ((address != BMP280_ADDR_1 && address != BMP280_ADDR_2) ||
        !is_i2c_initialized()) {
        return NULL;
    }

    bmp280_t *bmp = calloc(1, sizeof(bmp280_t));

    if (!bmp) {
        return NULL;
    }

    bmp->address = address;
    bmp->name = name;

    uint8_t chip_id;

    if (read_register(I2C_MASTER_NUM, address, 0xD0, &chip_id, 1) != ESP_OK) {
        free(bmp);
        return NULL;
    }

    // A BMP280 identifies itself with chip ID 0x58.
    if (chip_id != 0x58) {
        free(bmp);
        return NULL;
    }

    // Load the factory calibration values used by the compensation formulas.
    uint8_t calibration[24];

    if (read_register(I2C_MASTER_NUM, address, 0x88, calibration, sizeof(calibration)) != ESP_OK) {
        free(bmp);
        return NULL;
    }

    bmp->dig_t1 = read_u16(&calibration[0]);
    bmp->dig_t2 = read_i16(&calibration[2]);
    bmp->dig_t3 = read_i16(&calibration[4]);

    bmp->dig_p1 = read_u16(&calibration[6]);
    bmp->dig_p2 = read_i16(&calibration[8]);
    bmp->dig_p3 = read_i16(&calibration[10]);
    bmp->dig_p4 = read_i16(&calibration[12]);
    bmp->dig_p5 = read_i16(&calibration[14]);
    bmp->dig_p6 = read_i16(&calibration[16]);
    bmp->dig_p7 = read_i16(&calibration[18]);
    bmp->dig_p8 = read_i16(&calibration[20]);
    bmp->dig_p9 = read_i16(&calibration[22]);

    // CTRL_MEAS: temperature x2, pressure x4, normal mode.
    if (write_register(I2C_MASTER_NUM, address, 0xF4, 0x4F) != ESP_OK) {
        free(bmp);
        return NULL;
    }

    // CONFIG: 62.5 ms standby time and IIR filter coefficient 4.
    if (write_register(I2C_MASTER_NUM, address, 0xF5, 0x28) != ESP_OK) {
        free(bmp);
        return NULL;
    }

    bmp->initialized = true;

    // Store the first valid altitude as the reference for relative measurements.
    bmp->reference_altitude_m = bmp280_get_absolute_altitude(bmp);
    if (!isfinite(bmp->reference_altitude_m)) {
        free(bmp);
        return NULL;
    }

    return bmp;
}

////////////////////////////////////////////////////////////////////////////////
//////                           PUBLIC API                               //////
////////////////////////////////////////////////////////////////////////////////

bool bmp280_is_initialized(const bmp280_t *bmp) {
    if (!bmp) {
        return false;
    }

    return bmp->initialized;
}

float bmp280_get_absolute_altitude(bmp280_t *bmp) {
    if (!bmp || !bmp->initialized) {
        return NAN;
    }

    // Return NAN when the pressure read fails or produces an invalid value.
    float pressure_pa = get_pressure(bmp);
    return bmp280_pressure_to_altitude(pressure_pa, BMP280_SEA_LEVEL_PA);
}

float bmp280_get_relative_altitude(bmp280_t *bmp) {
    if (!bmp || !bmp->initialized || !isfinite(bmp->reference_altitude_m)) {
        return NAN;
    }

    // Relative altitude is measured from the pressure captured at initialization.
    float absolute_altitude_m = bmp280_get_absolute_altitude(bmp);
    if (!isfinite(absolute_altitude_m)) {
        return NAN;
    }

    return absolute_altitude_m - bmp->reference_altitude_m;
}