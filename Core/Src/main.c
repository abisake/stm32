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
#include "stm32f4xx.h"

// Systick
static volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void)
{
	ms_ticks++; // will be fired every 1 ms
}


void delay_ms(uint32_t ms)
{
	uint32_t start = ms_ticks;
	while ((ms_ticks - start) < ms);
}

// Configure the clock for 180 MHz
void clock_init(void)
{
	/* Flash wait states should be configured first -> FLASH_ACR, sEC 3.8.1, pAGE 80
	 * And for 180 MHz, Table 5, 5 ws is required
	 * Eanble prefetch - bit 8
	 * Instr. cache - bit 9
	 * Data cache - bit 10
	 * */
	FLASH->ACR = FLASH_ACR_LATENCY_5WS
				| FLASH_ACR_PRFTEN
				| FLASH_ACR_ICEN
				| FLASH_ACR_DCEN;

	// check it
	while((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_5WS);

	/* Configure PLL (RCC->PLLCFGR, page 128 & 129 Sec. 6.3.2
	* M=8, N=180, P=2, Q=4, will convert 16 MHz to 180 MHz
	* fVCO = 16/8 * 180 = 360 MHz  →  SYSCLK = 360/2 = 180 MHz
	* fUSB = 360/4 = 90 MHz (USB needs 48 MHz, will need separate PLLSAI)
	*/
	RCC->PLLCFGR = (8U << 0) // PLLM = 8   — bits[5:0]
                         | (180U <<  6)   // PLLN = 180 — bits[14:6]
                         | (0U   << 16)   // PLLP = 2   — bits[17:16]: 00=÷2
                         | (0U   << 22)   // PLLSRC=HSI — bit 22: 0=HSI
                         | (4U   << 24);  // PLLQ = 4   — bits[27:24]

	/* Enable PLL and wait for lock RCC->CR
	 * bit 24 = PLLON
	 * bit 25 = PLLRDY
	*/
	RCC->CR |= RCC_CR_PLLON;
	while(!(RCC->CR & RCC_CR_PLLRDY));

	/* Set AHB/APB prescalers (RCC_CFGR page 130, Sec 6.3.6)
	 * AHB /1 = HCLK = 180 MHz (HPRE bits[7:4] -> 0000)
	 * APB1 /4 = PCLK1 = 45 MHz (PPRE1 bits[12:10] -> 101)
	 * APB2 /2 = PCLK2 = 90 MHz (PPRE2 bits[15:13] = 100)
	*/
	RCC->CFGR |= RCC_CFGR_PPRE1_DIV4 // apb1 PRESCALER
			| RCC_CFGR_PPRE2_DIV2;	 // apb2 prescaler

	/* Change the clock from SYCLK to PLL (RCC_CFGR - page 130, Sec 6.3.6)
	 * 10: PLL used as the system clock
	 * and confirm SWS
	 */
	RCC->CFGR |= RCC_CFGR_SW_PLL;
	while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

}

// SysTick init
void systick_init(void)
{
	/* HCLK = 180 MHz, instead it will be HCLK/8 = 22.5 MHz
	 * For 1 milliseconds = (2,25,00,000 / 1000) - 1 = 22499
	 * SysTick is 24-bits, so 180 MHz = 1,67,77,215 which will fit
	 * in 24 bits, but in this code it HCLK/8
	 * Find the structure in core_cm4.h -> SysTick_Type
	 * */
	SysTick->LOAD = 22499U;	// set value
	SysTick->VAL = 0U;		// clear current value

	// CTRL: bit0=ENABLE, bit1=TICKINT (interrupt), bit2=CLKSRC (0=HCLK/8)
	SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
}

/* Configure the button presS
 * In STM32FR446E, B1 is wired to PC13 (schematic diagram), button pressed = 0
 * The pin has an external pull-up in the board, no need to configure internal
 * (Need to study)
*/
// Optional functionality
//uint8_t button_pressed(void)
//{
//	/* IDR - Input Data Register (page 187, Sec 7.4.5)
//	 * bitx = PCx -> bit13 = PC13
//	 * Pressed=0, Released=1
//	*/
//	return !(GPIOC->IDR & (1U << 13));
//}

/* Debounce is important
 * button switches bounces -> means they make or break the contact in the circuit
 * if debounce is not addressed, for MCU it might look like we are doing 40 time in 20 ms
 * */
uint8_t button_debounce(void)
{
	// how to address it, add a delay, when the pin goes low
	if(!(GPIOA->IDR & (1U << 13)))
	{
		delay_ms(20);
		return !(GPIOA->IDR & (1U << 13));
	}
	return 0;
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN*/
  /* Configure the clock
   */
  clock_init();
  systick_init();
  /* Step 1: Enable clock */
  /* Table 1. Page 55. GPIOA, GPIOC pin is mapped to AHB1 bus.
   * Page 143. Sec 6.3.10 Enable peripheral clock RCC_AHB1ENR
   * GPIOA in 0th bit, GPIOC in 2th bit
  */
  RCC->AHB1ENR |= (1U << 0) // setting 0th bit
		  	   | (1U << 2);


  /* Step 2: Configure LED2 pin
   * a. Find the pins for LED2
   * e. The Green LED will be connected to PA5 (schematic file)
   * PA5 ->[11:10]
   */
  GPIOA->MODER &= ~(0x3U << 10);
  GPIOA->MODER |= (0x1U << 10);

  /* PC13 = input (button). Reset state of MODER is 00 = input.
   * The Nucleo board has an external pull-up, so we leave PUPDR alone.
   */

  while(1)
  {
	  GPIOA->BSRR = (1U << 5);			// LED on
	  delay_ms(500);
	  GPIOA->BSRR = (1U << (5+16));		// LED off
	  delay_ms(500);

	  if (button_debounce())
	  {
		  delay_ms(100);
	  }
  }
/* USER CODE END*/
}
