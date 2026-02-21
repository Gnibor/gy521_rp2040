#include "gy521.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define GY521_REG_PWR_MGMT_1 0x6B
#define GY521_REG_ACCEL_XOUT_H 0x3B
#define GY521_REG_GYRO_XOUT_H  0x43

void gy521_init(void) {
    i2c_init(GY521_I2C_PORT, 400 * 1000);
    gpio_set_function(GY521_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(GY521_SCL_PIN, GPIO_FUNC_I2C);

#if GY521_USE_PULLUP
    gpio_pull_up(GY521_SDA_PIN);
    gpio_pull_up(GY521_SCL_PIN);
#endif

    if (GY521_INT_PIN >= 0) {
        gpio_init(GY521_INT_PIN);
        gpio_set_dir(GY521_INT_PIN, GPIO_IN);
    }

    // Wake up device
    uint8_t buf[2] = {GY521_REG_PWR_MGMT_1, 0};
    i2c_write_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, buf, 2, false);
}

bool gy521_test_connection(void) {
    uint8_t who_am_i = 0;
    i2c_write_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, (uint8_t[]){0x75}, 1, true);
    i2c_read_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, &who_am_i, 1, false);
    return who_am_i == 0x68;
}

void gy521_read_raw(int16_t* ax, int16_t* ay, int16_t* az,
                    int16_t* gx, int16_t* gy, int16_t* gz) {

    uint8_t data[14];
    i2c_write_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, (uint8_t[]){GY521_REG_ACCEL_XOUT_H}, 1, true);
    i2c_read_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, data, 14, false);

    *ax = (data[0] << 8) | data[1];
    *ay = (data[2] << 8) | data[3];
    *az = (data[4] << 8) | data[5];
    *gx = (data[8] << 8) | data[9];
    *gy = (data[10] << 8) | data[11];
    *gz = (data[12] << 8) | data[13];
}
