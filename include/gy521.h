#pragma once
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdint.h>
#include <sys/types.h>

// === Konfigurierbare Hardware ===
#ifndef GY521_I2C_PORT
#define GY521_I2C_PORT i2c1
#endif

#ifndef GY521_SDA_PIN
#define GY521_SDA_PIN 6   // Default, kann überschrieben werden
#endif

#ifndef GY521_SCL_PIN
#define GY521_SCL_PIN 7   // Default, kann überschrieben werden
#endif

#ifndef GY521_USE_PULLUP
#define GY521_USE_PULLUP 0 // 1=Pullup aktiv, 0=kein Pullup
#endif

#ifndef GY521_INT_PIN
#define GY521_INT_PIN 24  // Optional Interrupt-Pin
#endif

#ifndef GY521_I2C_ADDR
#define GY521_I2C_ADDR 0x68
#endif

/*
 * If a function can do something for
 * Accelerometer, Gyroscope (accel_gyro)
 * or Temperatur (accel_temp_gyro)
 * independently use one of these,
 * or 1,2,3. Or do whatever you like 
 * i'm not your mom.
*/
#define GY521_ACCEL 1
#define GY521_TEMP 2
#define GY521_GYRO 3

// Bitmasks for the Full-Scale-Range Select
#define GY521_ACCEL_FSR_SEL_2G 0x00
#define GY521_ACCEL_FSR_SEL_4G 0x08
#define GY521_ACCEL_FSR_SEL_8G 0x10
#define GY521_ACCEL_FSR_SEL_16G 0x18

#define GY521_GYRO_FSR_SEL_250DPS 0x00
#define GY521_GYRO_FSR_SEL_500DPS 0x08
#define GY521_GYRO_FSR_SEL_1000DPS 0x10
#define GY521_GYRO_FSR_SEL_2000DPS 0x18

// ==========================
// === Struct declaration ===
// ==========================
/*
 */
typedef struct {
	int16_t x,y,z;
} gy521_axis_raw_t;

typedef struct {
	float x,y,z;
} gy521_axis_scaled_t;

// GY-521 device struct
typedef struct gy521_s{
	struct{ // Values
		struct{ // ACCELerometer
			gy521_axis_raw_t raw;
			gy521_axis_scaled_t g;
		} accel;

		struct{ // GYROscope
			gy521_axis_raw_t raw;
			gy521_axis_scaled_t dps;
		} gyro;

		struct{ // TEMPeratur
			int16_t raw;
			float celsius;
		} temp;
	} v;

	struct{ // CONFig
		bool sleep;
		uint8_t clksel;
		bool reset;

		struct{ // ACCELerometer
			bool sleep;
			uint8_t fsr;

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

		struct{ // TEMPeratur
			bool disable;
		} temp;

		struct{ // GYROscope
			bool sleep;
			uint8_t fsr;
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

	struct{ // FuNctions
		bool (*test_connection)(void);
		bool (*sleep)(bool);
		bool (*read)(struct gy521_s (*), uint8_t, bool);
		bool (*set_fsr)(struct gy521_s (*));

		struct{ // ACCELerometer
			bool (*sleep)(uint8_t);
		} accel;

		struct{ // TEMPeratur
			bool (*sleep)(uint8_t);
		} temp;

		struct { // GYROscope
			bool (*calibrate)(uint8_t);
			bool (*sleep)(uint8_t);
		} gyro;
	} fn;
} gy521_s;

// ============================
// === Function declaration ===
// ============================
/*
 * gy521_init(); is to initialize the I²C connection with the Constants defined at the top.
 * And it gives you a struct where every other function and values are defined and read.
 */
gy521_s gy521_init(void);
bool gy521_test_connection(void);
bool gy521_read_reg(uint8_t reg, uint8_t *out, uint8_t how_many);
bool gy521_sleep(bool aktiv); // Takes a 'true' to aktivate and 'false' to deaktivate
/*
 * gy521_set_fsr(options); is to set the Full-Scale-Range.
 * it takes the values to set from the gy521.opt struct.
 */
bool gy521_set_fsr(gy521_s *gy521);
bool gy521_calibrate_gyro(uint8_t sample); // sample = how many samples should be taken (i use 10-15)
bool gy521_read(gy521_s *out, uint8_t accel_temp_gyro, bool scaled); // accel_temp_gyro = 0: read all | scaled = true: calculates raw in the G-Force/°C
