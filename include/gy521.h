#pragma once
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdint.h>
#include <sys/types.h>

// =============================
// === Configurable Hardware ===
// =============================
#ifndef GY521_I2C_PORT
#define GY521_I2C_PORT i2c1 // Default I2C port
#endif

#ifndef GY521_SDA_PIN
#define GY521_SDA_PIN 6    // Default SDA pin (can be overridden)
#endif

#ifndef GY521_SCL_PIN
#define GY521_SCL_PIN 7   // Default SCL pin (can be overridden)
#endif

#ifndef GY521_USE_PULLUP
#define GY521_USE_PULLUP 0 // 1 = enable internal pull-up, 0 = disabled
#endif

#ifndef GY521_INT_PIN
#define GY521_INT_PIN 24  // Optional interrupt pin
#endif

#ifndef GY521_I2C_ADDR
#define GY521_I2C_ADDR 0x68 // Default I2C address for GY-521(MPU-6050)
#endif

/*
 * Identifiers for selecting specific sensor blocks
 * Used in gy521_read()
 *
 * 0 = read all
 * 1 = accel only
 * 2 = temperature only
 * 3 = gyro only
 */
#define GY521_ALL 0
#define GY521_ACCEL 1
#define GY521_TEMP 2
#define GY521_GYRO 3

// ========================================
// === Full Scale Range (FSR) bit masks ===
// ========================================
#define GY521_ACCEL_FSR_SEL_2G 0x00
#define GY521_ACCEL_FSR_SEL_4G 0x08
#define GY521_ACCEL_FSR_SEL_8G 0x10
#define GY521_ACCEL_FSR_SEL_16G 0x18

#define GY521_GYRO_FSR_SEL_250DPS 0x00
#define GY521_GYRO_FSR_SEL_500DPS 0x08
#define GY521_GYRO_FSR_SEL_1000DPS 0x10
#define GY521_GYRO_FSR_SEL_2000DPS 0x18


#define GY521_CLKSEL_8MHZ 0x00
#define GY521_CLKSEL_GYRO_X 0x01
#define GY521_CLKSEL_GYRO_Y 0x02
#define GY521_CLKSEL_GYRO_Z 0x03
#define GY521_CLKSEL_EXT_32_768KHZ 0x04
#define GY521_CLKSEL_EXT_19_2MHZ 0x05
#define GY521_CLKSEL_STOP 0x07

#define GY521_LP_WAKE_CTRL_1_25HZ 0
#define GY521_LP_WAKE_CTRL_5HZ (0x01 << 6)
#define GY521_LP_WAKE_CTRL_20HZ (0x02 << 6)
#define GY521_LP_WAKE_CTRL_40HZ (0x03 << 6)

// =======================
// === Data Structures ===
// =======================

/*
 * Raw axis values directly from registers
 * Signed 16-bit values from sensor
 */
typedef struct {
	int16_t x,y,z;
} gy521_axis_raw_t;

/*
 * Scaled axis values
 * - Accel: g-force
 * - Gyro: degrees per second
 */
typedef struct {
	float x,y,z;
} gy521_axis_scaled_t;

/*
 * Main device structure
 *
 * This struct contains:
 * - Measured values
 * - Configuration state
 * - Function pointers (pseudo OOP style)
 */
typedef struct gy521_s{
	// =====================
	// === Sensor Values ===
	// =====================
	struct{ // Values
		struct{
			gy521_axis_raw_t raw; // Raw accelerometer values
			gy521_axis_scaled_t g; // Converted acceleration in G
		} accel;

		struct{
			gy521_axis_raw_t raw; // Raw gyro values
			gy521_axis_scaled_t dps; // Converted gyro in °/s
		} gyro;

		struct{
			int16_t raw; // Raw temperature register
			float celsius; // Converted temperature in °C
		} temp;
	} v;

	// =====================
	// === Configuration ===
	// =====================
	struct{
		bool sleep; // Device sleep state
		uint8_t clksel; // Clock source
		bool reset; // Device reset flag

		struct{
			uint8_t fsr; // Full scale range setting
			float fsr_divider;

			struct{
				bool stby;
			} x;

			struct{
				bool stby;
			} y;

			struct{
				bool stby;
			} z;
		} accel;

		struct{
			bool disable; // Disable temperature sensor
		} temp;

		struct{
			uint8_t fsr;
			float fsr_divider;
			uint8_t calibrate_samples;

			struct{
				bool clksel;
				bool stby;
			} x;

			struct{
				bool clksel;
				bool stby;
			} y;

			struct{
				bool clksel;
				bool stby;
			} z;
		} gyro;
	} conf;

	// =========================
	// === Function Pointers ===
	// =========================
	struct{
		bool (*test_connection)(void);
		bool (*sleep)(bool);
		bool (*read)(struct gy521_s (*), uint8_t, bool);
		bool (*set_fsr)(struct gy521_s (*));

		struct{
			bool (*sleep)(uint8_t);
		} accel;

		struct{
			bool (*sleep)(uint8_t);
		} temp;

		struct{
			bool (*calibrate)(uint8_t);
			bool (*sleep)(uint8_t);
		} gyro;
	} fn;
} gy521_s;

// ============================
// === Function declaration ===
// ============================
/*
 * gy521_init();
 * Initializes the I²C connection and default configuration.
 * Returns a fully initialized gy521_s struct with function pointers and default values.
 */
gy521_s gy521_init(void);
