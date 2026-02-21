#pragma once
#include "pico/stdlib.h"
#include "hardware/i2c.h"

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
#define GY521_USE_PULLUP 1 // 1=Pullup aktiv, 0=kein Pullup
#endif

#ifndef GY521_INT_PIN
#define GY521_INT_PIN 24  // Optional Interrupt-Pin
#endif

#ifndef GY521_I2C_ADDR
#define GY521_I2C_ADDR 0x68
#endif

// === Funktionsdeklarationen ===
void gy521_init(void);
bool gy521_test_connection(void);
void gy521_read_raw(int16_t* ax, int16_t* ay, int16_t* az,
                    int16_t* gx, int16_t* gy, int16_t* gz);
