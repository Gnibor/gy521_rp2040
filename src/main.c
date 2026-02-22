// include pico headers
#include "pico/stdlib.h"
#include <stdio.h>
#include "gy521.h"
#include "default.h"

gy521_scaled_t gy521;



int main(void){
	stdio_init_board();
	printf("test\n");
	gy521_init();

	int retries = 3;
	bool connected = false;

	while (retries--){
		connected = gy521_test_connection();
		if(connected) break;

		printf("Retrying...\n");
		sleep_ms(750);
	}

	if(!connected){
		printf("GY-521 nicht gefunden!\n");
	}

	printf("GY-521 bereit!\n");

	bool ret = gy521_set_fs(GY521_ACCEL_FS, GY521_ACCEL_FS_SEL_4G);
	while(!ret) tight_loop_contents();
	ret = gy521_set_fs(GY521_GYRO_FS, GY521_GYRO_FS_SEL_1000DPS);
	while(!ret) tight_loop_contents();

	while(1){
		if(gy521_read_scaled(&gy521))
			printf("G=X:%6.3f Y:%6.3f Z:%6.3f | °C=%6.2f | °/s=X:%8.3f Y:%8.3f Z:%8.3f\n", 
				gy521.ax, gy521.ay, gy521.az, gy521.temp, gy521.gx, gy521.gy, gy521.gz);
		sleep_ms(250);
	}
	return 0;
}
