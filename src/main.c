/*
 * ================================================================
 *  Project:      GY-521 (MPU-6050) Driver Example for RP2040
 *  File:         main.c
 *  Author:       (Gnibor) Robin Gerhartz
 *  License:      MIT License
 *  Repository:   https://github.com/Gnibor/gy521_rp2040
 * ================================================================
 *
 *  Description:
 *  Example application demonstrating how to initialize,
 *  configure and read data from the GY-521 (MPU-6050)
 *  accelerometer + gyroscope module using the RP2040.
 *
 *  Features demonstrated:
 *  - I²C initialization
 *  - Device detection (WHO_AM_I check)
 *  - Device reset and wake-up
 *  - Full-scale range configuration
 *  - Clock source selection
 *  - Axis standby control
 *  - Gyroscope calibration
 *  - Continuous sensor readout (scaled output)
 *
 *  This file is meant as a usage example for the gy521 driver.
 *
 * ================================================================
 */
#include "pico/stdlib.h"
#include <stdio.h>
#include <sys/types.h>

#include "default.h"
#include "gy521.h"

int main(void){
	stdio_init_board();
	gy521_s gy521 = gy521_init();
	int retries = 3;
	bool connected = false;
	while(retries--){
		printf("Try connecting GY-521...\n");
		connected = gy521.fn.test_connection();
		if(connected) break;

		printf("Retrying...\n");
		sleep_ms(750);
	}
	if(!connected) printf("GY-521 not found!\n");
	else printf("GY-521 ready!\n");

	if(gy521.fn.reset()) printf("GY-521 got reset\n");

	if(gy521.fn.sleep(false)) printf("GY-521 sleep stop\n");

	gy521.conf.accel.fsr = GY521_ACCEL_FSR_SEL_8G;
	gy521.conf.gyro.fsr = GY521_GYRO_FSR_SEL_2000DPS;
	if(gy521.fn.set_fsr(&gy521)) printf("GY-521 Full-Scale-Range is set.\n");

	gy521.conf.gyro.x.clksel = true;
	if(gy521.fn.clksel(&gy521)) printf("GY-521 Clock Select set to GyroX\n");

	gy521.conf.accel.y.stby = true;
	if(gy521.fn.set_stby(&gy521)) printf("YA in standby\n");

	printf("Try to calibrate GY-521\n");
	sleep_ms(2000);
	if(gy521.fn.gyro.calibrate(15)) printf("GY-521 is now calibrated.\n");
	else printf("GY-521 could not be calibrated.\n");

	while(1){
		if(gy521.fn.read(&gy521, 0, true))
			printf("G=X:%6.3f Y:%6.3f Z:%6.3f | °C=%6.2f | °/s=X:%9.3f Y:%9.3f Z:%9.3f\n", 
				gy521.v.accel.g.x, gy521.v.accel.g.y, gy521.v.accel.g.z, 
				gy521.v.temp.celsius, 
				gy521.v.gyro.dps.x, gy521.v.gyro.dps.y, gy521.v.gyro.dps.z);
		sleep_ms(500);
	}
}
#include "pico/stdlib.h"
#include <stdio.h>
#include <sys/types.h>

#include "default.h"
#include "gy521.h"

int main(void){
	stdio_init_board();
	gy521_s gy521 = gy521_init();
	int retries = 3;
	bool connected = false;
	while(retries--){
		printf("Try connecting GY-521...\n");
		connected = gy521.fn.test_connection();
		if(connected) break;

		printf("Retrying...\n");
		sleep_ms(750);
	}
	if(!connected) printf("GY-521 not found!\n");
	else printf("GY-521 ready!\n");

	if(gy521.fn.reset()) printf("GY-521 got reset\n");

	if(gy521.fn.sleep(false)) printf("GY-521 sleep stop\n");

	gy521.conf.accel.fsr = GY521_ACCEL_FSR_SEL_8G;
	gy521.conf.gyro.fsr = GY521_GYRO_FSR_SEL_2000DPS;
	if(gy521.fn.set_fsr(&gy521)) printf("GY-521 Full-Scale-Range is set.\n");

	gy521.conf.gyro.x.clksel = true;
	if(gy521.fn.clksel(&gy521)) printf("GY-521 Clock Select set to GyroX\n");

	gy521.conf.accel.y.stby = true;
	if(gy521.fn.set_stby(&gy521)) printf("YA in standby\n");

	printf("Try to calibrate GY-521\n");
	sleep_ms(2000);
	if(gy521.fn.gyro.calibrate(15)) printf("GY-521 is now calibrated.\n");
	else printf("GY-521 could not be calibrated.\n");

	while(1){
		if(gy521.fn.read(&gy521, 0, true))
			printf("G=X:%6.3f Y:%6.3f Z:%6.3f | °C=%6.2f | °/s=X:%9.3f Y:%9.3f Z:%9.3f\n", 
				gy521.v.accel.g.x, gy521.v.accel.g.y, gy521.v.accel.g.z, 
				gy521.v.temp.celsius, 
				gy521.v.gyro.dps.x, gy521.v.gyro.dps.y, gy521.v.gyro.dps.z);
		sleep_ms(500);
	}
}
