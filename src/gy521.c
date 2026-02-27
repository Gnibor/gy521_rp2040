#include "pico/stdlib.h"
#include "hardware/i2c.h"
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

// Function prototypes
bool gy521_test_connection(void);
bool gy521_read_reg(uint8_t reg, uint8_t *out, uint8_t how_many);
bool gy521_sleep(bool aktiv); // true = enable, false = disable
bool gy521_set_fsr(gy521_s *gy521); // automatically calculate scaling factors
bool gy521_set_clksel(gy521_s *gy521);
bool gy521_calibrate_gyro(uint8_t sample); // calibrate gyro offsets
bool gy521_read(gy521_s *out, uint8_t accel_temp_gyro, bool scaled); // 0=all, scaled=true -> G/°C/°/s

typedef struct {
	int32_t x, y, z;
} gy521_offset_t;

// ========================
// === Global Variables ===
// ========================
static gy521_offset_t gy521_offset; // gyro offsets for zero-point
static uint8_t gy521_cache[14]={0}; // temporary buffer for I2C reads

// ========================
// === Initialize GY521 ===
// ========================
gy521_s gy521_init(void) {
	i2c_init(GY521_I2C_PORT, 400 * 1000); // 400 kHz I2C
	gpio_set_function(GY521_SDA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(GY521_SCL_PIN, GPIO_FUNC_I2C);

#if GY521_USE_PULLUP
	gpio_pull_up(GY521_SDA_PIN);
	gpio_pull_up(GY521_SCL_PIN);
#endif

	// Configure optional interrupt pin
	if (GY521_INT_PIN >= 0) {
		gpio_init(GY521_INT_PIN);
		gpio_set_dir(GY521_INT_PIN, GPIO_IN);
	}

	// Initalize device struct and function pointers
	gy521_s gy521 = {0};
	gy521.conf.accel.fsr_divider = 131.0f;
	gy521.conf.gyro.fsr_divider = 16384;
	gy521.conf.gyro.x.clksel = true;
	gy521.fn.sleep = &gy521_sleep;
	gy521.fn.test_connection = &gy521_test_connection;
	gy521.fn.read = &gy521_read;
	gy521.fn.gyro.calibrate = &gy521_calibrate_gyro;
	gy521.fn.set_fsr = &gy521_set_fsr;
	gy521.fn.clksel = &gy521_set_clksel;
	return gy521;
}

// =========================
// === I2C Register Read ===
// =========================
bool gy521_read_register(uint8_t reg, uint8_t *out, uint8_t how_many){
	uint8_t ret = i2c_write_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, (uint8_t[]){reg}, 1, true);
	if(ret < 0) return false;

	ret = i2c_read_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, out, how_many, false);
	if(ret < 0) return false;

	return true;
}

// =======================
// === Test Connection ===
// =======================
bool gy521_test_connection(void) {
	uint8_t who_am_i;
	if(!gy521_read_register(GY521_REG_WHO_AM_I, &who_am_i, 1)) return false;
	return who_am_i == 0x68 ? true : false;
}

bool gy521_set_clksel(gy521_s *gy521){
	if(gy521->conf.clksel == 0){
		if(gy521->conf.gyro.x.clksel) gy521->conf.clksel = GY521_CLKSEL_GYRO_X;
		else if(gy521->conf.gyro.y.clksel) gy521->conf.clksel = GY521_CLKSEL_GYRO_Y;
		else if (gy521->conf.gyro.z.clksel) gy521->conf.clksel = GY521_CLKSEL_GYRO_Z;
	}

	if(!gy521_read_register(GY521_REG_PWR_MGMT_1, gy521_cache, 1)) return false;
	gy521_cache[0] &= ~0x07;
	gy521_cache[0] |= gy521->conf.clksel;

	int ret = i2c_write_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, (uint8_t[]){ GY521_REG_PWR_MGMT_1, gy521_cache[0]}, 2, false);
	if(ret < 0) return false;

	return true;
}

// ==================
// === Sleep Mode ===
// ==================
bool gy521_sleep(bool aktiv){
	uint8_t reg_entry = 0x00;
	
	if(!gy521_read_register(GY521_REG_PWR_MGMT_1, &reg_entry, 1)) return false;

	bool is_set = reg_entry & GY521_SLEEP;

	if(aktiv && !is_set) reg_entry |= GY521_SLEEP; // Enable sleep
	else if(!aktiv) reg_entry &= ~GY521_SLEEP; // Disable sleep
	else return true;

	// Write back to register
	gy521_cache[0] = GY521_REG_PWR_MGMT_1;
	gy521_cache[1] = reg_entry;
	int ret = i2c_write_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, gy521_cache, 2, false);
	if(ret < 0) return false;

	return true;
}

// ===================================
// ===  Set Full-Scale Range (FSR) ===
// === & Calculate Scaling Factors ===
// ===================================
bool gy521_set_fsr(gy521_s *gy521){


	// Read FSR Register
	uint8_t reg[2];
	if(!gy521_read_register(GY521_REG_GYRO_CONFIG, reg, 2)) return false;

	// Gyro FSR bits
	reg[0] &= ~(0x18); // Delete bits 4:3
	reg[0] |= gy521->conf.gyro.fsr; // Set FSR Bits

	// Automatic scaling calculation:
	// 131 / 2^bits → sensitivity in °/s
	gy521->conf.gyro.fsr_divider = 131.0f / (1 << ((gy521->conf.gyro.fsr >> 3) & 0x03));

	// Accel FSR bits
	reg[1] &= ~(0x18);
	reg[1] |= gy521->conf.accel.fsr;

	// Automatic scaling calculation (raw / divider = G)
	gy521->conf.accel.fsr_divider = 16384.0f / (1 << ((gy521->conf.accel.fsr >> 3) & 0x03));

	// Write back to registers
	int ret = i2c_write_blocking(GY521_I2C_PORT, GY521_I2C_ADDR, (uint8_t[]){GY521_REG_GYRO_CONFIG, reg[0], reg[1]}, 3, false);
	if(ret < 0) return false;

	return true;
}

// ====================================================
// === Calibrate gyro (determine zero-point offset) ===
// ====================================================
bool gy521_calibrate_gyro(uint8_t samples){
	gy521_axis_raw_t raw;
	
	int64_t sum_gx = 0;
	int64_t sum_gy = 0;
	int64_t sum_gz = 0;

	for (uint8_t i = 0; i < samples; i++){
		if(!gy521_read_register(GY521_REG_GYRO_XOUT_H, gy521_cache, 6)) return false;

		raw.x = (gy521_cache[0]  << 8) | gy521_cache[1];
		raw.y = (gy521_cache[2]  << 8) | gy521_cache[3];
		raw.z = (gy521_cache[4]  << 8) | gy521_cache[5];

		sum_gx += raw.x;
		sum_gy += raw.y;
		sum_gz += raw.z;

		sleep_ms(5); // small delay between measurements
	}

	// Store averages as offsets
	gy521_offset.x = sum_gx / samples;
	gy521_offset.y = sum_gy / samples;
	gy521_offset.z = sum_gz / samples;

	return true;
}

// ===========================================
// === Read Sensor Data + Optional Scaling ===
// ===========================================
bool gy521_read(gy521_s *gy521, uint8_t accel_temp_gyro, bool scaled) {
	// Read all sensors
	if(accel_temp_gyro == 0){
		if(!gy521_read_register(GY521_REG_ACCEL_XOUT_H, gy521_cache, 14)) return false;

		gy521->v.accel.raw.x = (gy521_cache[0]  << 8) | gy521_cache[1];
		gy521->v.accel.raw.y = (gy521_cache[2]  << 8) | gy521_cache[3];
		gy521->v.accel.raw.z = (gy521_cache[4]  << 8) | gy521_cache[5];
		gy521->v.temp.raw = (gy521_cache[6]  << 8) | gy521_cache[7];
		gy521->v.gyro.raw.x = (gy521_cache[8]  << 8) | gy521_cache[9];
		gy521->v.gyro.raw.y = (gy521_cache[10] << 8) | gy521_cache[11];
		gy521->v.gyro.raw.z = (gy521_cache[12] << 8) | gy521_cache[13];

	// Only accelerometer
	}else if(accel_temp_gyro == 1){
		if(!gy521_read_register(GY521_REG_ACCEL_XOUT_H, gy521_cache, 6)) return false;

		gy521->v.accel.raw.x = (gy521_cache[0]  << 8) | gy521_cache[1];
		gy521->v.accel.raw.y = (gy521_cache[2]  << 8) | gy521_cache[3];
		gy521->v.accel.raw.z = (gy521_cache[4]  << 8) | gy521_cache[5];

	// Only temperatur
	}else if(accel_temp_gyro == 2){
		if(!gy521_read_register(GY521_REG_TEMP_OUT_H, gy521_cache, 2)) return false;

		gy521->v.temp.raw = (gy521_cache[0]  << 8) | gy521_cache[1];

	// Only gyroscope
	}else if(accel_temp_gyro == 3){
		if(!gy521_read_register(GY521_REG_GYRO_XOUT_H, gy521_cache, 6)) return false;

		gy521->v.gyro.raw.x = (gy521_cache[0]  << 8) | gy521_cache[1];
		gy521->v.gyro.raw.y = (gy521_cache[2] << 8) | gy521_cache[3];
		gy521->v.gyro.raw.z = (gy521_cache[4] << 8) | gy521_cache[5];
	}

	// Optional: scale raw values
	if(scaled){
		// Raw -> G for accelerometer
		if(accel_temp_gyro == 0 || accel_temp_gyro == 1){
			gy521->v.accel.g.x = gy521->v.accel.raw.x / gy521->conf.accel.fsr_divider;
			gy521->v.accel.g.y = gy521->v.accel.raw.y / gy521->conf.accel.fsr_divider;
			gy521->v.accel.g.z = gy521->v.accel.raw.z / gy521->conf.accel.fsr_divider;
		}

		// Raw -> °C
		if(accel_temp_gyro == 0 || accel_temp_gyro == 2)
			gy521->v.temp.celsius = (gy521->v.temp.raw / 340.0f) + 36.53f;

		// Raw -> °/s for gyroscope
		if(accel_temp_gyro == 0 || accel_temp_gyro == 3){
			gy521->v.gyro.dps.x = (gy521->v.gyro.raw.x - gy521_offset.x) / gy521->conf.gyro.fsr_divider;
			gy521->v.gyro.dps.y = (gy521->v.gyro.raw.y - gy521_offset.y) / gy521->conf.gyro.fsr_divider;
			gy521->v.gyro.dps.z = (gy521->v.gyro.raw.z - gy521_offset.z) / gy521->conf.gyro.fsr_divider;
		}
	}

	return true;
}
