# GY-521 (MPU-6050) Driver for RP2040

Lightweight C driver for the **GY-521 (MPU-6050)** 6-axis IMU designed for the **Raspberry Pi RP2040** (Raspberry Pi Pico). Provides clean register-level implementation with automatic scaling and structured device API using function pointers (object-oriented style in C).

---

## Project Note
This is my first project in almost a decade. Focus is on:

- Clean low-level implementation  
- Transparent register control  
- Minimal abstraction  
- Explicit configuration  
- Expandability toward full MPU-6050 feature coverage  

---

## Requirements
- Raspberry Pi pico-sdk  
- RP2040 toolchain  
- CMake-based build environment  

Dependencies: pico/stdlib, hardware/i2c  

---

## Badges
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)  
![Platform: RP2040](https://img.shields.io/badge/Platform-RP2040-blue)  
![Language: C](https://img.shields.io/badge/Language-C-informational)  
![Interface: I2C](https://img.shields.io/badge/Interface-I2C-orange)  

---

## Features (Current Implementation)
- I²C communication (400 kHz)  
- WHO_AM_I device verification  
- Device reset and wake-up  
- Clock source selection  
- Accelerometer & Gyroscope Full-Scale-Range configuration  
- Standby control per axis  
- Gyroscope zero-offset calibration  
- Raw + scaled sensor output: Acceleration in **g**, Angular velocity in **°/s**, Temperature in **°C**  
- No dynamic memory allocation  
- Fully configurable via macros  

---

## Development Plan

### Interrupt Configuration & Handling
- INT_ENABLE register configuration  
- INT_STATUS decoding  
- Data-ready and motion detection interrupt support  
- External GPIO interrupt integration  
- Interrupt-driven sampling  

### FIFO Support
- FIFO_EN configuration  
- FIFO_COUNT handling  
- Burst read from FIFO  
- Continuous buffered sampling mode  

### I2C Slave (I2C_SLVx) Configuration
- External sensor passthrough  
- I2C_SLV0–I2C_SLV4 setup  
- Master mode configuration  

### Extended Power Management
- Cycle mode support  
- Low-power wake control  
- Temperature sensor disable  
- Fine-grained standby control API  

### Complete Register Coverage
- Structured access to all MPU-6050 registers  
- Optional register debug dump function  

---

## Hardware
- Raspberry Pi Pico / RP2040  
- GY-521 (MPU-6050)  
- I²C wiring (default): SDA → GPIO 6, SCL → GPIO 7  
- Default I²C address: 0x68  

---

## Configuration
Hardware config can be adjusted in `gy521.h` or in `main.c` before `#include "gy521.h"`:

```c
#define GY521_I2C_PORT i2c1
#define GY521_SDA_PIN 6
#define GY521_SCL_PIN 7
#define GY521_USE_PULLUP 0
#include "gy521.h"
```

---

## Basic Usage

```c
int main(void)
{
    stdio_usb_init();
    while (!stdio_usb_connected()) sleep_ms(100);

    gy521_s imu = gy521_init(GY521_I2C_ADDR_GND);

    if (!imu.fn.test_connection()) {
        printf("Device not found!\n");
        return 1;
    }

    imu.conf.sleep = false;
    imu.conf.temp.sleep = false;
    imu.fn.sleep();
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
gy521_s gy521_init(uint8_t addr);
```

Initializes I²C and returns a fully configured device struct.

### Switch Device

```c
bool gy521_use(gy521_s device);
```

Sets a global pointer to 'device' all .fn. are now bounded to this 'device'.

---

### Core Functions

| Function | Description |
|----------|------------|
| `gy521_init(addr)` | Initilize I²C connection and returns a device struct |
| `gy521_use(device)` | Set the global pointer for fn.* to 'device' |
| `fn.test_connection()` | Verifies device via WHO_AM_I register |
| `fn.reset()` | Performs device reset |
| `fn.sleep()` | Enables/disables sleep mode |
| `fn.fsr()` | Sets full-scale range and updates scaling |
| `fn.stby()` | Enables/disables standby per axis |
| `fn.clksel()` | Selects clock source |
| `fn.read()` | Reads sensor data (raw or scaled) |
| `fn.gyro.calibrate(samples)` | Computes gyro zero-offset |

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

Contributions (especially with I²C_SLV and FIFO), suggestions and improvements are welcome.
