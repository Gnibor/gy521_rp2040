#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdint.h>
#include "gy521.h"

#define GY521_REG_ACCEL_XOUT_H 0x3B
#define GY521_REG_ACCEL_CONFIG 0x1c
#define GY521_REG_TEMP_OUT_H 0x41
#define GY521_REG_GYRO_XOUT_H  0x43
#define GY521_REG_GYRO_CONFIG 0x1b
#define GY521_REG_WHO_AM_I 0x75

#define GY521_REG_SIGNAL_PATH_RESET 0x68
#define GY521_GYRO_RESET (1 << 2)
#define GY521_ACCEL_RESET (1 << 1)
#define GY521_TEMP_RESET 0x01

#define GY521_REG_USER_CTRL 0x6A
#define GY521_FIFO_EN (1 << 6)
#define GY521_I2C_MST_EN (1 << 5)
#define GY521_I2C_IF_DIS (1 << 4)
#define GY521_FIFO_RESET (1 << 2)
#define GY521_I2C_MST_RESET (1 << 1)
#define GY521_SIG_COND_RESET 0x01

#define GY521_REG_PWR_MGMT_1 0x6B
#define GY521_DEVICE_RESET (1 << 7)
#define GY521_SLEEP (1 << 6) // 0=deaktivate, 1=aktivate
#define GY521_CYCLE (1 << 5)
#define GY521_TEMP_DIS (1 << 3)
#define GY521_CLKSEL_8MHZ 0x00
#define GY521_CLKSEL_GYRO_X 0x01
#define GY521_CLKSEL_GYRO_Y 0x02
#define GY521_CLKSEL_GYRO_Z 0x03
#define GY521_CLKSEL_EXT_32_768KHZ 0x04
#define GY521_CLKSEL_EXT_19_2MHZ 0x05
#define GY521_CLKSEL_STOP 0x07

#define GY521_REG_PWR_MGMT_2 0x6C
#define GY521_LP_WAKE_CTRL_1_25HZ 0
#define GY521_LP_WAKE_CTRL_5HZ (0x01 << 6)
#define GY521_LP_WAKE_CTRL_20HZ (0x02 << 6)
#define GY521_LP_WAKE_CTRL_40HZ (0x03 << 6)
#define GY521_STBY_XA (1 << 5)
#define GY521_STBY_YA (1 << 4)
#define GY521_STBY_ZA (1 << 3)
#define GY521_STBY_XG (1 << 2)
#define GY521_STBY_YG (1 << 1)
#define GY521_STBY_ZG 0x01

#define GY521_ACCEL_FSR_DIV_2G 16384.0f
#define GY521_ACCEL_FSR_DIV_4G 8129.0f
#define GY521_ACCEL_FSR_DIV_8G 4096.0f
#define GY521_ACCEL_FSR_DIV_16G 2048.0f

#define GY521_GYRO_FSR_DIV_250DPS 131.0f
#define GY521_GYRO_FSR_DIV_500DPS 65.5f
#define GY521_GYRO_FSR_DIV_1000DPS 32.8f
#define GY521_GYRO_FSR_DIV_2000DPS 16.4f

typedef struct {
	int32_t x, y, z;
} gy521_offset_t;

static gy521_offset_t gy521_offset;

static float gy521_gyro_fsr_div = GY521_GYRO_FSR_DIV_250DPS;
static float gy521_accel_fsr_div = GY521_ACCEL_FSR_DIV_2G;

static uint8_t gy521_cache[14]={0};

gy521_s gy521_init(void) {
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

	// Initalize device struct.
	gy521_s gy521 = {0};
	gy521.fn.sleep = &gy521_sleep;
	gy521.fn.test_connection = &gy521_test_connection;
	gy521.fn.read = &gy521_read;
	gy521.fn.gyro.calibrate = &gy521_calibrate_gyro;
	gy521.fn.set_fsr = &gy521_set_fsr;
	return gy521;
}

bool gy521_read_reg(uint8_t reg, uint8_t *out, uint8_t how_many){
	int ret = i2c_write_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, (uint8_t[]){reg}, 1, true);
	if(ret < 0) return false;

	ret = i2c_read_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, out, how_many, false);
	if(ret < 0) return false;

	return true;
}

bool gy521_test_connection(void) {
	uint8_t who_am_i;
	if(!gy521_read_reg(GY521_REG_WHO_AM_I, &who_am_i, 1)) return false;
	return who_am_i == 0x68 ? true : false;
}

bool gy521_sleep(bool aktiv){
	uint8_t reg_entry = 0x00;
	
	if(!gy521_read_reg(GY521_REG_PWR_MGMT_1, &reg_entry, 1)) return false;

	if(aktiv) reg_entry |= GY521_SLEEP;
	else reg_entry ^= GY521_SLEEP;
	
	uint8_t buf[2] = {GY521_REG_PWR_MGMT_1, reg_entry};
	int ret = i2c_write_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, buf, 2, false);
	if(ret < 0) return false;

	return true;
}

bool gy521_set_fsr(gy521_s *gy521){
	if(gy521->conf.gyro.fsr == GY521_GYRO_FSR_SEL_500DPS) gy521_gyro_fsr_div = GY521_GYRO_FSR_DIV_500DPS;
	else if(gy521->conf.gyro.fsr == GY521_GYRO_FSR_SEL_1000DPS) gy521_gyro_fsr_div = GY521_GYRO_FSR_DIV_1000DPS;
	else if(gy521->conf.gyro.fsr == GY521_GYRO_FSR_SEL_2000DPS) gy521_gyro_fsr_div = GY521_GYRO_FSR_DIV_2000DPS;
	else gy521_gyro_fsr_div = GY521_GYRO_FSR_DIV_250DPS;

	if(gy521->conf.accel.fsr == GY521_ACCEL_FSR_SEL_4G) gy521_accel_fsr_div = GY521_ACCEL_FSR_DIV_4G;
	else if(gy521->conf.accel.fsr == GY521_ACCEL_FSR_SEL_8G) gy521_accel_fsr_div = GY521_ACCEL_FSR_DIV_8G;
	else if(gy521->conf.accel.fsr == GY521_ACCEL_FSR_SEL_16G) gy521_accel_fsr_div = GY521_ACCEL_FSR_DIV_16G;
	else gy521_accel_fsr_div = GY521_ACCEL_FSR_DIV_2G;

	int ret = i2c_write_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, (uint8_t[]){GY521_REG_GYRO_CONFIG, gy521->conf.gyro.fsr, gy521->conf.accel.fsr}, 3, false);
	if(ret < 0) return false;

	return true;
}

bool gy521_calibrate_gyro(uint8_t samples){
	gy521_axis_raw_t raw;
	
	int64_t sum_gx = 0;
	int64_t sum_gy = 0;
	int64_t sum_gz = 0;

	for (int i = 0; i < samples; i++){
		if(!gy521_read_reg(GY521_REG_GYRO_XOUT_H, gy521_cache, 6)) return false;

		raw.x = (gy521_cache[0]  << 8) | gy521_cache[1];
		raw.y = (gy521_cache[2]  << 8) | gy521_cache[3];
		raw.z = (gy521_cache[4]  << 8) | gy521_cache[5];

		sum_gx += raw.x;
		sum_gy += raw.y;
		sum_gz += raw.z;

		sleep_ms(5);
	}

	gy521_offset.x = sum_gx / samples;
	gy521_offset.y = sum_gy / samples;
	gy521_offset.z = sum_gz / samples;

	return true;
}

bool gy521_read(gy521_s *gy521, uint8_t accel_temp_gyro, bool scaled) {
	if(accel_temp_gyro == 0){
		if(!gy521_read_reg(GY521_REG_ACCEL_XOUT_H, gy521_cache, 14)) return false;

		gy521->v.accel.raw.x = (gy521_cache[0]  << 8) | gy521_cache[1];
		gy521->v.accel.raw.y = (gy521_cache[2]  << 8) | gy521_cache[3];
		gy521->v.accel.raw.z = (gy521_cache[4]  << 8) | gy521_cache[5];
		gy521->v.temp.raw = (gy521_cache[6]  << 8) | gy521_cache[7];
		gy521->v.gyro.raw.x = (gy521_cache[8]  << 8) | gy521_cache[9];
		gy521->v.gyro.raw.y = (gy521_cache[10] << 8) | gy521_cache[11];
		gy521->v.gyro.raw.z = (gy521_cache[12] << 8) | gy521_cache[13];
	}else if(accel_temp_gyro == 1){
		if(!gy521_read_reg(GY521_REG_ACCEL_XOUT_H, gy521_cache, 6)) return false;

		gy521->v.accel.raw.x = (gy521_cache[0]  << 8) | gy521_cache[1];
		gy521->v.accel.raw.y = (gy521_cache[2]  << 8) | gy521_cache[3];
		gy521->v.accel.raw.z = (gy521_cache[4]  << 8) | gy521_cache[5];

	}else if(accel_temp_gyro == 2){
		if(!gy521_read_reg(GY521_REG_TEMP_OUT_H, gy521_cache, 2)) return false;

		gy521->v.temp.raw = (gy521_cache[0]  << 8) | gy521_cache[1];

	}else if(accel_temp_gyro == 3){
		if(!gy521_read_reg(GY521_REG_GYRO_XOUT_H, gy521_cache, 6)) return false;

		gy521->v.gyro.raw.x = (gy521_cache[0]  << 8) | gy521_cache[1];
		gy521->v.gyro.raw.y = (gy521_cache[2] << 8) | gy521_cache[3];
		gy521->v.gyro.raw.z = (gy521_cache[4] << 8) | gy521_cache[5];
	}

	if(scaled){
		if(accel_temp_gyro == 0 || accel_temp_gyro == 1){
			gy521->v.accel.g.x = gy521->v.accel.raw.x / gy521_accel_fsr_div;
			gy521->v.accel.g.y = gy521->v.accel.raw.y / gy521_accel_fsr_div;
			gy521->v.accel.g.z = gy521->v.accel.raw.z / gy521_accel_fsr_div;
		}

		if(accel_temp_gyro == 0 || accel_temp_gyro == 2)
			gy521->v.temp.celsius = (gy521->v.temp.raw / 340.0f) + 36.53f;

		if(accel_temp_gyro == 0 || accel_temp_gyro == 3){
			gy521->v.gyro.dps.x = (gy521->v.gyro.raw.x - gy521_offset.x) / gy521_gyro_fsr_div;
			gy521->v.gyro.dps.y = (gy521->v.gyro.raw.y - gy521_offset.y) / gy521_gyro_fsr_div;
			gy521->v.gyro.dps.z = (gy521->v.gyro.raw.z - gy521_offset.z) / gy521_gyro_fsr_div;
		}
	}

	return true;
}
