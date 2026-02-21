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

int main(void) {
	stdio_init_board();
	printf("test");
	gy521_init();

	if (!gy521_test_connection()) {
		printf("GY-521 nicht gefunden!\n");
		while (1) tight_loop_contents();
	}

	printf("GY-521 bereit!\n");

	while (1) {
		int16_t ax, ay, az, gx, gy, gz;
		gy521_read_raw(&ax, &ay, &az, &gx, &gy, &gz);
		printf("A: %6d %6d %6d | G: %6d %6d %6d\n", ax, ay, az, gx, gy, gz);
		sleep_ms(500);
	}
}
