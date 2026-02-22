// include pico headers
#include "pico/stdlib.h"
#include <stdio.h>
#include "gy521.h"

// Optionale Defines für Konfiguration
#ifndef USE_USB
#define USE_USB 1       // 1 = stdio über USB, 0 = über UART
#endif

#ifndef USE_UART
#define USE_UART 0      // 1 = UART aktivieren, nur wenn USE_USB=0
#endif

void stdio_init_board(void) {
#if USE_USB
    stdio_usb_init();
    // warten, bis USB verbunden ist
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
#elif USE_UART
    stdio_uart_init();
#endif
}


gy521_scaled_t gy521;



int main(void) {
	stdio_init_board();
	printf("test\n");
	gy521_init();

	int retries = 3;
	bool connected = false;
	while (!(connected = gy521_test_connection()) && retries--){
		printf("Retrying...\n");
		sleep_ms(500);
	}
	if (retries < 0) {
		printf("GY-521 nicht gefunden!\n");
		while(1) tight_loop_contents();
	}

	printf("GY-521 bereit!\n");

	bool ret = gy521_set_fs(GY521_AFS, GY521_AFS_SEL_4);
	while(!ret) tight_loop_contents();
	ret = gy521_set_fs(GY521_GFS, GY521_GFS_SEL_1000);
	while(!ret) tight_loop_contents();

	while (1) {
		if(gy521_read_scaled(&gy521))
			printf("G=X:%6.3f Y:%6.3f Z:%6.3f | °C=%6.2f | °/s=X:%8.3f Y:%8.3f Z:%8.3f\n", 
				gy521.ax, gy521.ay, gy521.az, gy521.temp, gy521.gx, gy521.gy, gy521.gz);
		sleep_ms(250);
	}
}
