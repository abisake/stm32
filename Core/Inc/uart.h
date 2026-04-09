#pragma once


#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

void uart_init(void);
void uart_putchar(char ch);
void uart_puts(const char *str);
bool uart_rx_available(void);
char uart_rx_read(void);
