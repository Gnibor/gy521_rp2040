#pragma once
// Optionale Defines für Konfiguration
#ifndef USE_USB
#define USE_USB 1       // 1 = stdio über USB, 0 = über UART
#endif

#ifndef USE_UART
#define USE_UART 0      // 1 = UART aktivieren, nur wenn USE_USB=0
#endif

void stdio_init_board(void);
