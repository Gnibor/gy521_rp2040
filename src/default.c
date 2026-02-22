#include "pico/stdlib.h"
#include "default.h"

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
