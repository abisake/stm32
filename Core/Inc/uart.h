#pragma once


#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

void uart_init(void);
void uart_dma_init(void);
void uart_dma_send(const uint8_t *buf, uint16_t len);
