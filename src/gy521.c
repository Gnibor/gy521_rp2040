#include "gy521.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define GY521_REG_PWR_MGMT_1 0x6B
#define GY521_REG_ACCEL_XOUT_H 0x3B
#define GY521_REG_GYRO_XOUT_H  0x43
#define GY521_REG_GYRO_CONFIG 0x1b
#define GY521_REG_ACCEL_CONFIG 0x1c
#define GY521_REG_WHO_AM_I 0x75

#define GY521_GFS_DIV_250 131.0f
#define GY521_GFS_DIV_500 65.5f
#define GY521_GFS_DIV_1000 32.8f
#define GY521_GFS_DIV_2000 16.4f

#define GY521_AFS_DIV_2 16384.0f
#define GY521_AFS_DIV_4 8129.0f
#define GY521_AFS_DIV_8 4096.0f
#define GY521_AFS_DIV_16 2048.0f

static float gy521_gfs_div = GY521_GFS_DIV_250;
static float gy521_afs_div = GY521_AFS_DIV_2;

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
	i2c_write_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, (uint8_t[]){GY521_REG_WHO_AM_I}, 1, true);
	i2c_read_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, &who_am_i, 1, false);
	return who_am_i == 0x68;
}

bool gy521_set_fs(bool xfs, u_int16_t xfs_sel){
	u_int16_t addr = GY521_REG_ACCEL_CONFIG;
	if(xfs){
		addr = GY521_REG_GYRO_CONFIG;
		if(xfs_sel == GY521_GFS_SEL_500) gy521_gfs_div = GY521_GFS_DIV_500;
		else if(xfs_sel == GY521_GFS_SEL_1000) gy521_gfs_div = GY521_GFS_DIV_1000;
		else if(xfs_sel == GY521_GFS_SEL_2000) gy521_gfs_div = GY521_GFS_DIV_2000;
		else gy521_gfs_div = GY521_GFS_DIV_250;
	}else{
		if(xfs_sel == GY521_AFS_SEL_4) gy521_afs_div = GY521_AFS_DIV_4;
		else if(xfs_sel == GY521_AFS_SEL_8) gy521_afs_div = GY521_AFS_DIV_8;
		else if(xfs_sel == GY521_AFS_SEL_16) gy521_afs_div = GY521_AFS_DIV_16;
		else gy521_afs_div = GY521_AFS_DIV_2;
	}

	int ret = i2c_write_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, (uint8_t[]){addr, xfs_sel}, 2, true);
	
	if(ret < 0) return 0;

	return true;
}

bool gy521_read_raw(gy521_raw_t *out) {
	uint8_t data[14];

	int ret = i2c_write_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, (uint8_t[]){GY521_REG_ACCEL_XOUT_H}, 1, true);

	if (ret < 0) return false;

	ret = i2c_read_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, data, 14, false);

	if (ret < 0) return false;

	out->ax   = (data[0]  << 8) | data[1];
	out->ay   = (data[2]  << 8) | data[3];
	out->az   = (data[4]  << 8) | data[5];
	out->temp = (data[6]  << 8) | data[7];
	out->gx   = (data[8]  << 8) | data[9];
	out->gy   = (data[10] << 8) | data[11];
	out->gz   = (data[12] << 8) | data[13];

	return true;
}

bool gy521_read_scaled(gy521_scaled_t *out){
	gy521_raw_t imu;
	if (gy521_read_raw(&imu)) {
		out->ax = imu.ax / gy521_afs_div;
		out->ay = imu.ay / gy521_afs_div;
		out->az = imu.az / gy521_afs_div;
		out->gx = imu.gx / gy521_gfs_div;
		out->gy = imu.gy / gy521_gfs_div;
		out->gz = imu.gz / gy521_gfs_div;
		out->temp = (imu.temp / 340.0f) + 36.53f;
		return true;
	}
	return false;
}
