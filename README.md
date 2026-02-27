# GY-521 (MPU-6050) Driver for RP2040

Lightweight C driver for the **GY-521 (MPU-6050)** 6-axis IMU  
designed for the **Raspberry Pi RP2040** (Raspberry Pi Pico).

This project provides a clean, register-level implementation with
automatic scaling and a structured device API using function pointers
(object-oriented style in C).

---

## Project Note

This is my first project in almost a decade. Please be kind.
It is still a work in progress and actively being extended and refined.

The focus is on:

- Clean low-level implementation
- Transparent register control
- Minimal abstraction
- Explicit configuration
- Expandability toward full MPU-6050 feature coverage

---

## Requirements

This project requires the **Raspberry Pi pico-sdk**.

You must have:

- RP2040 toolchain
- pico-sdk installed and configured
- CMake-based build environment

Official SDK repository:
https://github.com/raspberrypi/pico-sdk

This driver depends on:

- `pico/stdlib`
- `hardware/i2c`

It will not compile without the pico-sdk.

---

## Badges

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![Platform: RP2040](https://img.shields.io/badge/Platform-RP2040-blue)
![Language: C](https://img.shields.io/badge/Language-C-informational)
![Interface: I2C](https://img.shields.io/badge/Interface-I2C-orange)

---

## Features (Current Implementation)

- I²C communication (400 kHz default)
- WHO_AM_I device verification
- Device reset and wake-up control
- Clock source selection
- Accelerometer & Gyroscope Full-Scale-Range configuration
- Standby control per axis
- Gyroscope zero-offset calibration
- Raw + scaled sensor output:
  - Acceleration in **g**
  - Angular velocity in **°/s**
  - Temperature in **°C**
- No dynamic memory allocation
- Fully configurable via macros

---

## Development Roadmap

The following features are planned and will be implemented eventuell. (Not in order)

### 1. Interrupt Configuration & Handling

Planned additions:

- INT_ENABLE register configuration
- INT_STATUS decoding
- Data-ready interrupt support
- Optional motion detection interrupt
- External interrupt pin integration (GPIO IRQ handler)
- Interrupt-driven sampling instead of polling

Goal:
Allow event-driven sensor reading with minimal CPU overhead.

---

### 2. FIFO Support

Planned additions:

- FIFO_EN configuration
- FIFO_COUNT register handling
- Burst read from FIFO
- Overflow detection
- Optional continuous buffered sampling mode

Goal:
Enable higher sample rates and buffered data acquisition.

---

### 3. I2C Slave (I2C_SLVx) Configuration

Planned additions:

- External sensor passthrough configuration
- I2C_SLV0–I2C_SLV4 setup
- Master mode configuration
- Automatic data injection into FIFO
- Secondary sensor integration support

Goal:
Support advanced MPU-6050 internal I2C master features.

---

### 4. Extended Power Management

Planned additions:

- Cycle mode support
- Low-power wake control configuration
- Temperature sensor disable option
- Fine-grained standby control API

---

### 5. Complete Register Coverage

Long-term goal:

- Structured access to all MPU-6050 registers
- Improved configuration abstraction
- Optional register debug dump function

---

## Hardware

- Raspberry Pi Pico / RP2040
- GY-521 (MPU-6050)
- I²C wiring:
  - SDA → configurable (default GPIO 6)
  - SCL → configurable (default GPIO 7)

Default I²C address: `0x68`

---

## Configuration

Hardware configuration can be adjusted in `gy521.h`
or in `main.c` befor the `#include 'gy521.h'`:

```c
#define GY521_I2C_PORT i2c1
#define GY521_SDA_PIN 6
#define GY521_SCL_PIN 7
#define GY521_I2C_ADDR 0x68
#define GY521_USE_PULLUP 0
```

---

## Basic Usage

```c
#include "gy521.h"

int main(void)
{
    stdio_usb_init();
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    gy521_s imu = gy521_init();

    if (!imu.fn.test_connection()) {
        printf("Device not found!\n");
        return 1;
    }

    imu.fn.sleep(false);
    imu.conf.accel.fsr = GY521_ACCEL_FSR_SEL_4G;
    imu.conf.gyro.fsr = GY521_GYRO_FSR_SEL_1000DPS;
    imu.fn.set_fsr(&imu);

    imu.fn.gyro.calibrate(20);

    while (1) {
        if (imu.fn.read(&imu, GY521_ALL, true)) {
            printf("Accel: %.2f %.2f %.2f g\n",
                   imu.v.accel.g.x,
                   imu.v.accel.g.y,
                   imu.v.accel.g.z);
        }
        sleep_ms(500);
    }
}
```

---

## API Overview

### Initialization

```c
gy521_s gy521_init(void);
```

Initializes I²C and returns a fully configured device struct.

---

### Core Functions

| Function | Description |
|----------|------------|
| `test_connection()` | Verifies device via WHO_AM_I register |
| `reset()` | Performs device reset |
| `sleep(bool)` | Enables/disables sleep mode |
| `set_fsr()` | Sets full-scale range and updates scaling |
| `set_stby()` | Enables standby per axis |
| `clksel()` | Selects clock source |
| `read()` | Reads sensor data (raw or scaled) |
| `gyro.calibrate(samples)` | Computes gyro zero-offset |

---

## Scaling

Raw sensor values are automatically converted when `scaled = true`.

### Accelerometer

```
g = raw / fsr_divider
```

### Gyroscope

```
°/s = (raw - offset) / fsr_divider
```

### Temperature

```
°C = (raw / 340) + 36.53
```

---

## Design Philosophy

This driver:

- Avoids dynamic memory
- Avoids hidden global state (except calibration offset)
- Uses explicit configuration
- Provides register-level transparency
- Emulates object-oriented behavior using structured function pointers

The goal is clarity, control and minimal abstraction for embedded systems.

---

## License

MIT License

Copyright (c) 2026  
(Gnibor) Robin Gerhartz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.

---

## Repository

https://github.com/Gnibor/gy521_rp2040

---

## Status

Early but functional implementation.  
Actively developed toward full MPU-6050 feature support.

Contributions, suggestions and improvements are welcome.
