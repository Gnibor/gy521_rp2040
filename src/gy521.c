/*
 * ================================================================
 *  Project:      GY-521 (MPU-6050) Driver for RP2040
 *  File:         gy521.c
 *  Author:       (Gnibor) Robin Gerhartz
 *  License:      MIT License
 *  Repository:   https://github.com/Gnibor/gy521_rp2040
 * ================================================================
 *
 *  Description:
 *  Low-level driver implementation for the GY-521 module
 *  based on the MPU-6050 6-axis IMU sensor.
 *
 *  This file implements:
 *  - I²C communication
 *  - Register-level configuration
 *  - Sensor data acquisition
 *  - Automatic scaling (raw -> physical units)
 *  - Gyroscope zero-point calibration
 *  - Power management features
 *
 *  The driver is written in a lightweight embedded style
 *  and uses function pointers inside a device structure
 *  to emulate object-oriented behavior in C.
 *
 * ================================================================
 */
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdint.h>
#include "gy521.h"

// ==========================================
// === GY521(MPU-6050) register addresses ===
// ==========================================
#define GY521_REG_ACCEL_XOUT_H 0x3B
#define GY521_REG_ACCEL_CONFIG 0x1c
#define GY521_REG_TEMP_OUT_H 0x41
#define GY521_REG_GYRO_XOUT_H  0x43
#define GY521_REG_GYRO_CONFIG 0x1b
#define GY521_REG_SIGNAL_PATH_RESET 0x68
#define GY521_REG_USER_CTRL 0x6A
#define GY521_REG_PWR_MGMT_1 0x6B
#define GY521_REG_PWR_MGMT_2 0x6C
#define GY521_REG_WHO_AM_I 0x75

// =============================================
// === Bitmasks for reset, FIFO, sleep, etc. ===
// =============================================
#define GY521_GYRO_RESET (1 << 2)
#define GY521_ACCEL_RESET (1 << 1)
#define GY521_TEMP_RESET 0x01

#define GY521_FIFO_EN (1 << 6)
#define GY521_I2C_MST_EN (1 << 5)
#define GY521_I2C_IF_DIS (1 << 4)
#define GY521_FIFO_RESET (1 << 2)
#define GY521_I2C_MST_RESET (1 << 1)
#define GY521_SIG_COND_RESET 0x01

#define GY521_DEVICE_RESET (1 << 7)
#define GY521_SLEEP (1 << 6) // Sleep mode enable/disable
#define GY521_CYCLE (1 << 5)
#define GY521_TEMP_DIS (1 << 3)

#define GY521_STBY_XA (1 << 5)
#define GY521_STBY_YA (1 << 4)
#define GY521_STBY_ZA (1 << 3)
#define GY521_STBY_XG (1 << 2)
#define GY521_STBY_YG (1 << 1)
#define GY521_STBY_ZG 0x01

// ===========================
// === Function prototypes ===
// ===========================
bool gy521_test_connection(void);
bool gy521_read_reg(uint8_t reg, uint8_t *out, uint8_t how_many);
bool gy521_reset(void);
bool gy521_sleep(void); // true = enable, false = disable
bool gy521_set_fsr(void); // automatically calculate scaling factors
bool gy521_set_clksel(void);
bool gy521_set_stby(void);
bool gy521_calibrate_gyro(uint8_t sample); // calibrate gyro offsets
bool gy521_read(uint8_t accel_temp_gyro); // 0=all 1=accel 2=temp 3=gyro

// ========================
// === Global Variables ===
// ========================
// Globaler Pointer auf das aktive GY521-Device
static gy521_s *g_gy521 = NULL;

static uint8_t g_gy521_cache[14]={0}; // temporary buffer for I2C reads
static int g_gy521_ret_cache = 0;

// =========================
// === Set device to use ===
// =========================
// to set this device as used device for fn.
bool gy521_use(gy521_s *device) {
	// Prüfe ob Pointer gültig ist
	if (device == NULL) {
		return false;
	}
	g_gy521 = device;

	return true;
}

// ========================
// === Initialize GY521 ===
// ========================
gy521_s gy521_init(uint8_t addr){
	i2c_init(GY521_I2C_PORT, 400 * 1000); // 400 kHz I2C
	gpio_set_function(GY521_SDA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(GY521_SCL_PIN, GPIO_FUNC_I2C);

#if GY521_USE_PULLUP
	gpio_pull_up(GY521_SDA_PIN);
	gpio_pull_up(GY521_SCL_PIN);
#endif

	// Configure optional interrupt pin
	if (GY521_INT_PIN >= 0){
		gpio_init(GY521_INT_PIN);
		gpio_set_dir(GY521_INT_PIN, GPIO_IN);
	}

	// Initalize device struct and function pointers
	gy521_s gy521 = {0};

	if(!addr) gy521.conf.addr = GY521_I2C_ADDR_GND;
	else gy521.conf.addr = addr;

	gy521.conf.accel.fsr_divider = 131.0f;
	gy521.conf.gyro.fsr_divider = 16384.0f;
	gy521.conf.gyro.x.clksel = true;

	gy521.fn.reset = &gy521_reset;
	gy521.fn.sleep = &gy521_sleep;
	gy521.fn.test_connection = &gy521_test_connection;
	gy521.fn.read = &gy521_read;
	gy521.fn.gyro.calibrate = &gy521_calibrate_gyro;
	gy521.fn.fsr = &gy521_set_fsr;
	gy521.fn.stby = &gy521_set_stby;
	gy521.fn.clk_sel = &gy521_set_clksel;

	g_gy521 = &gy521;

	return gy521;
}

// =========================
// === I2C Register Read ===
// =========================
bool gy521_read_register(uint8_t reg, uint8_t *out, uint8_t how_many){
	if(!g_gy521) return false;

	uint8_t g_gy521_ret_cache = i2c_write_blocking(GY521_I2C_PORT, g_gy521->conf.addr, (uint8_t[]){reg}, 1, true);
	if(g_gy521_ret_cache!= 1) return false;

	g_gy521_ret_cache = i2c_read_blocking(GY521_I2C_PORT, g_gy521->conf.addr, out, how_many, false);
	if(g_gy521_ret_cache!= how_many) return false;

	return true;
}

// =======================
// === Test Connection ===
// =======================
bool gy521_test_connection(void){
	uint8_t who_am_i;
	if(!gy521_read_register(GY521_REG_WHO_AM_I, &who_am_i, 1)) return false;
	return who_am_i == 0x68 ? true : false;
}

// ==========================
// === Reset GY521 device ===
// ==========================
bool gy521_reset(void){
	if(!g_gy521) return false;

	if(!gy521_read_register(GY521_REG_PWR_MGMT_1, g_gy521_cache, 1)) return false;
	g_gy521_cache[0] |= GY521_DEVICE_RESET;

	g_gy521_ret_cache = i2c_write_blocking(GY521_I2C_PORT, g_gy521->conf.addr, (uint8_t[]){GY521_REG_PWR_MGMT_1, g_gy521_cache[0]}, 2, false);
	if(g_gy521_ret_cache!= 2) return false;

	return true;
}

// ==========================================
// === Set Standby in Register PWR_MGMT_2 ===
// ==========================================
bool gy521_set_stby(void){
	if(!g_gy521) return false;
	if(!gy521_read_register(GY521_REG_PWR_MGMT_2, g_gy521_cache, 1)) return false;

	g_gy521_cache[0] &= ~0x3f;
	if(g_gy521->conf.gyro.x.stby) g_gy521_cache[0] |= GY521_STBY_XG;
	if(g_gy521->conf.gyro.y.stby) g_gy521_cache[0] |= GY521_STBY_YG;
	if(g_gy521->conf.gyro.z.stby) g_gy521_cache[0] |= GY521_STBY_ZG;

	if(g_gy521->conf.accel.x.stby) g_gy521_cache[0] |= GY521_STBY_XA;
	if(g_gy521->conf.accel.y.stby) g_gy521_cache[0] |= GY521_STBY_YA;
	if(g_gy521->conf.accel.z.stby) g_gy521_cache[0] |= GY521_STBY_ZA;

	g_gy521_ret_cache = i2c_write_blocking(GY521_I2C_PORT, g_gy521->conf.addr, (uint8_t[]){ GY521_REG_PWR_MGMT_2, g_gy521_cache[0]}, 2, false);
	if(g_gy521_ret_cache!= 2) return false;

	return true;
}

// ==========================================
// === Set CLK_SEL in Register PWR_MGMT_1 ===
// ==========================================
bool gy521_set_clksel(void){
	if(!g_gy521) return false;
	if(g_gy521->conf.gyro.x.clksel) g_gy521->conf.clksel = GY521_CLKSEL_GYRO_X;
	else if(g_gy521->conf.gyro.y.clksel) g_gy521->conf.clksel = GY521_CLKSEL_GYRO_Y;
	else if (g_gy521->conf.gyro.z.clksel) g_gy521->conf.clksel = GY521_CLKSEL_GYRO_Z;

	if(!gy521_read_register(GY521_REG_PWR_MGMT_1, g_gy521_cache, 1)) return false;
	g_gy521_cache[0] &= ~0x47; // clear sleep & CLK_SEL
	g_gy521_cache[0] |= g_gy521->conf.clksel;

	g_gy521_ret_cache = i2c_write_blocking(GY521_I2C_PORT, g_gy521->conf.addr, (uint8_t[]){ GY521_REG_PWR_MGMT_1, g_gy521_cache[0]}, 2, false);
	if(g_gy521_ret_cache!= 2) return false;

	return true;
}

// ==================
// === Sleep Mode ===
// ==================
bool gy521_sleep(void){
	if(!g_gy521) return false;
	if(!gy521_read_register(GY521_REG_PWR_MGMT_1, g_gy521_cache, 1)) return false;

	// Sleep Bit
	if(g_gy521->conf.sleep) g_gy521_cache[0] |= GY521_SLEEP;
	else g_gy521_cache[0] &= ~GY521_SLEEP;

	// Temperature disable Bit
	if(g_gy521->conf.temp.sleep) g_gy521_cache[0] |= GY521_TEMP_DIS;
	else g_gy521_cache[0] &= ~GY521_TEMP_DIS;

	g_gy521_ret_cache = i2c_write_blocking(GY521_I2C_PORT, g_gy521->conf.addr, (uint8_t[]){GY521_REG_PWR_MGMT_1, g_gy521_cache[0]}, 2, false);
	if(g_gy521_ret_cache!= 2) return false;

	return true;
}

// ===================================
// ===  Set Full-Scale Range (FSR) ===
// === & Calculate Scaling Factors ===
// ===================================
bool gy521_set_fsr(void){
	if(!g_gy521) return false;
	// Read FSR Register
	if(!gy521_read_register(GY521_REG_GYRO_CONFIG, g_gy521_cache, 2)) return false;

	// Gyro FSR bits
	g_gy521_cache[0] &= ~0x18; // Delete bits 4:3
	g_gy521_cache[0] |= g_gy521->conf.gyro.fsr; // Set FSR Bits

	// Automatic scaling calculation:
	// 131 / 2^bits → sensitivity in °/s
	g_gy521->conf.gyro.fsr_divider = 131.0f / (1 << ((g_gy521->conf.gyro.fsr >> 3) & 0x03));

	// Accel FSR bits
	g_gy521_cache[1] &= ~0x18;
	g_gy521_cache[1] |= g_gy521->conf.accel.fsr;

	// Automatic scaling calculation (raw / divider = G)
	g_gy521->conf.accel.fsr_divider = 16384.0f / (1 << ((g_gy521->conf.accel.fsr >> 3) & 0x03));

	// Write back to registers
	 g_gy521_ret_cache = i2c_write_blocking(GY521_I2C_PORT, g_gy521->conf.addr, (uint8_t[]){GY521_REG_GYRO_CONFIG, g_gy521_cache[0], g_gy521_cache[1]}, 3, false);
	if(g_gy521_ret_cache!= 3) return false;

	return true;
}

// ====================================================
// === Calibrate gyro (determine zero-point offset) ===
// ====================================================
bool gy521_calibrate_gyro(uint8_t samples){
	if(!g_gy521) return false;
	gy521_axis_raw_t raw;
	
	int64_t sum_gx = 0;
	int64_t sum_gy = 0;
	int64_t sum_gz = 0;

	for (uint8_t i = 0; i < samples; i++){
		if(!gy521_read_register(GY521_REG_GYRO_XOUT_H, g_gy521_cache, 6)) return false;

		raw.x = (g_gy521_cache[0]  << 8) | g_gy521_cache[1];
		raw.y = (g_gy521_cache[2]  << 8) | g_gy521_cache[3];
		raw.z = (g_gy521_cache[4]  << 8) | g_gy521_cache[5];

		sum_gx += raw.x;
		sum_gy += raw.y;
		sum_gz += raw.z;

		sleep_ms(5); // small delay between measurements
	}

	// Store averages as offsets
	g_gy521->conf.gyro.offset.x = sum_gx / samples;
	g_gy521->conf.gyro.offset.y = sum_gy / samples;
	g_gy521->conf.gyro.offset.z = sum_gz / samples;

	return true;
}

// ===========================================
// === Read Sensor Data + Optional Scaling ===
// ===========================================
bool gy521_read(uint8_t accel_temp_gyro){
	if(!g_gy521) return false;
	// Read all sensors
	if(accel_temp_gyro == 0){
		if(!gy521_read_register(GY521_REG_ACCEL_XOUT_H, g_gy521_cache, 14)) return false;

		g_gy521->v.accel.raw.x = (g_gy521_cache[0]  << 8) | g_gy521_cache[1];
		g_gy521->v.accel.raw.y = (g_gy521_cache[2]  << 8) | g_gy521_cache[3];
		g_gy521->v.accel.raw.z = (g_gy521_cache[4]  << 8) | g_gy521_cache[5];
		g_gy521->v.temp.raw = (g_gy521_cache[6]  << 8) | g_gy521_cache[7];
		g_gy521->v.gyro.raw.x = (g_gy521_cache[8]  << 8) | g_gy521_cache[9];
		g_gy521->v.gyro.raw.y = (g_gy521_cache[10] << 8) | g_gy521_cache[11];
		g_gy521->v.gyro.raw.z = (g_gy521_cache[12] << 8) | g_gy521_cache[13];

	// Only accelerometer
	}else if(accel_temp_gyro == 1){
		if(!gy521_read_register(GY521_REG_ACCEL_XOUT_H, g_gy521_cache, 6)) return false;

		g_gy521->v.accel.raw.x = (g_gy521_cache[0]  << 8) | g_gy521_cache[1];
		g_gy521->v.accel.raw.y = (g_gy521_cache[2]  << 8) | g_gy521_cache[3];
		g_gy521->v.accel.raw.z = (g_gy521_cache[4]  << 8) | g_gy521_cache[5];

	// Only temperatur
	}else if(accel_temp_gyro == 2){
		if(!gy521_read_register(GY521_REG_TEMP_OUT_H, g_gy521_cache, 2)) return false;

		g_gy521->v.temp.raw = (g_gy521_cache[0]  << 8) | g_gy521_cache[1];

	// Only gyroscope
	}else if(accel_temp_gyro == 3){
		if(!gy521_read_register(GY521_REG_GYRO_XOUT_H, g_gy521_cache, 6)) return false;

		g_gy521->v.gyro.raw.x = (g_gy521_cache[0]  << 8) | g_gy521_cache[1];
		g_gy521->v.gyro.raw.y = (g_gy521_cache[2] << 8) | g_gy521_cache[3];
		g_gy521->v.gyro.raw.z = (g_gy521_cache[4] << 8) | g_gy521_cache[5];
	}

	// Optional: scale raw values
	if(g_gy521->conf.scaled){
		// Raw -> G for accelerometer
		if(accel_temp_gyro == 0 || accel_temp_gyro == 1){
			g_gy521->v.accel.g.x = g_gy521->v.accel.raw.x / g_gy521->conf.accel.fsr_divider;
			g_gy521->v.accel.g.y = g_gy521->v.accel.raw.y / g_gy521->conf.accel.fsr_divider;
			g_gy521->v.accel.g.z = g_gy521->v.accel.raw.z / g_gy521->conf.accel.fsr_divider;
		}

		// Raw -> °C
		if(accel_temp_gyro == 0 || accel_temp_gyro == 2)
			g_gy521->v.temp.celsius = (g_gy521->v.temp.raw / 340.0f) + 36.53f;

		// Raw -> °/s for gyroscope
		if(accel_temp_gyro == 0 || accel_temp_gyro == 3){
			g_gy521->v.gyro.dps.x = (g_gy521->v.gyro.raw.x - g_gy521->conf.gyro.offset.x) / g_gy521->conf.gyro.fsr_divider;
			g_gy521->v.gyro.dps.y = (g_gy521->v.gyro.raw.y - g_gy521->conf.gyro.offset.y) / g_gy521->conf.gyro.fsr_divider;
			g_gy521->v.gyro.dps.z = (g_gy521->v.gyro.raw.z - g_gy521->conf.gyro.offset.z) / g_gy521->conf.gyro.fsr_divider;
		}
	}

	return true;
}
