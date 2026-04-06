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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
/* A UART function send one msg
*/
void uart_putchar(char msg)
{
	// Poll TXE bit (bit 7 of SR) — RM0390 page 812
	while (!(USART2->SR & (1U << 7)));
	USART2->DR = msg; // RM0390 page 814
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN*/
  /* Configure the clock */
  /* Step 1: Enable clock */
  /* Table 1. Page 55. GPIOA pin is mapped to AHB1 bus.
   * Page 143. Sec 6.3.10 Enable peripheral clock RCC_AHB1ENR
   * GPIOA in 0th bit
  */
  RCC->AHB1ENR |= (1U << 0); // setting 0th bit
  /* Table 1. Page 59. USART2 is mapped to APB1 bus
   * Page 145. Sec 6.3.13 Enable peripheral clock RCC_APB1ENR
   * USART2EN in 17th bit
  */
  RCC->APB1ENR |= (1U << 17); // setting 17th bit

  /* Step 2: Configure GPIO pins & LED2 pin
   * a. Find the pins for USART2 in the pinout diagram, PA2, PA3
   * b. MODER is 2 bit pin, so 2n+1:2 -> PA2=[5:4], PA3=[7:6]
   * c. Set PA2
  */
  // do not set the value directly, instead clear the field you want change
  // and set the value
  GPIOA->MODER &= ~(0x3U << 4); // USART2_TX
  GPIOA->MODER |= (0x2U << 4);

  /* d. Set the same for PA3 which is USART_RX
  */
  GPIOA->MODER &= ~(0x3U << 6);
  GPIOA->MODER |= (0x2U << 6);

  /* e. The Green LED will be connected to PA5 (schematic file)
   * PA5 ->[11:10]
   */
  GPIOA->MODER &= ~(0x3U << 10);
  GPIOA->MODER |= (0x1U << 10);

  /* Step 3: Select AF7 for PA2 & PA3
   * Table 11. Page.57 Data sheet PA2=AF7=USART2_TX, PA3=AF7=USART2_RX
   * GPIOA_AFRL page 189. Section 7.4.9 AF7=0b0111=0x7
   * AFRLy: Alternate function selection for port x bit y (y = 0..7)
   * PA2 [11:8] & PA3 [15:12]
  */
  GPIOA->AFR[0] &= ~(0xFU << 8);	// clear PA2 [11:8]
  GPIOA->AFR[0] |= (0x7U << 8); // PA2 = AF7
  GPIOA->AFR[0] &= ~(0xFU << 12); // clear PA3 [15:12]
  GPIOA->AFR[0] |= (0x7U << 12); // PA3 = AF7

  /* Step 4: Configure USART
   * BRR page 817 Section 25.6.3: 16 MHz HSI / 115200 baud = 138 = 0x8B
   * */
  USART2->BRR = 0x008B;
  /* CR1 page 815 Section 25.6.4: bit13=UE, bit3=TE, bit2=RE
  */
  USART2->CR1 = (1U << 3) | (1U << 2) | (1U << 13);

  /* Main loop
   * IN BSRR page 188, bits[15:0] for set, bits[31:16] for clear
   */
  while (1)
  {
	  GPIOA->BSRR = (1U << 5); // LED is ON
	  uart_putchar('1');
	  // volatile -> prevents compiler optimization, different from ATOMIC
	  for(volatile uint32_t i =0; i < 500000 ; i++);// milliseconds

	  GPIOA->BSRR = (1U << (5+16)); // LED is OFF
	  uart_putchar('0');
	  for(volatile uint32_t i =0; i < 500000 ; i++); // milliseconds
  }
}
/* USER CODE END*/
