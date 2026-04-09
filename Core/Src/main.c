/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "uart.h"

int main(void)
{
	uart_init();
	uart_puts("Ready......\r\n");

	while(1)
	{
		if(uart_rx_available())
		{
			// echo back whatever you type
			char msg = uart_rx_read();
			uart_putchar(msg);
		}
	}
}

