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
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stdio_ext.h"
#include "string.h"
#include "semphr.h"

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
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

static SemaphoreHandle_t xM1 = NULL;
static SemaphoreHandle_t xM2 = NULL;
static SemaphoreHandle_t xUartMutex = NULL;

static void uart_puts(const char *s)
{
	// If scheduler is not started mean not need for uart mutex
	if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
	{
		HAL_UART_Transmit(&huart2, (const uint8_t*)s, strlen(s), HAL_MAX_DELAY);
	}
	else
	{
	// scheduler is started, lock via mutex, send the data and give mutex
		if(xSemaphoreTake(xUartMutex, portMAX_DELAY) == pdTRUE)
		{
			HAL_UART_Transmit(&huart2, (const uint8_t*)s, strlen(s), HAL_MAX_DELAY);
			xSemaphoreGive(xUartMutex);
		}
	}
}

/* Simple code to print using uart instead of printf*/

//static void uart_print_u32(const char *ch, uint32_t value)
//{
//	char buffer[32];
//	char temp[12];
//
//	int i = 0, j = 0;
//
//	// "ch" msg
//	while(ch[i])
//	{
//		buffer[j++] = ch[i++];
//	}
//
//	// print the value
//	if(value == 0)
//	{
//		buffer[j++] = '0'; // simple case
//	}
//	else // TODO: Need to revist the below logic
//	{
//		int k = 0;
//		while (value > 0)
//		{
//			temp[k++] = '0' + (value % 10);
//			value /= 10;
//		}
//		while (k > 0)
//		{
//			buffer[j++] = temp[--k];
//		}
//	}
//
//	buffer[j++]='\r';
//	buffer[j++]='\n';
//	buffer[j] = 0;
//
//	uart_puts(buffer);
//}

// task 1, takes M1 and then M2
static void task_1(void *pvParams)
{
	vTaskDelay(pdMS_TO_TICKS(100)); // lets start in an order

	uart_puts("Task 1: -> started\r\n");

	while(1)
	{
		uart_puts("Task 1: Taking the M1\r\n");
		xSemaphoreTake(xM1, portMAX_DELAY);
		uart_puts("Task 1: Got M1\r\n");

		// adding delay for the task_2 take M2, assuming 100ms
		vTaskDelay(pdMS_TO_TICKS(100));

		// take M2
		uart_puts("Task 1: Taking M2\r\n");
		xSemaphoreTake(xM2, portMAX_DELAY); // deadlock will happen herr
		uart_puts("Task 1: Got M2\r\n");

		xSemaphoreGive(xM2);
		xSemaphoreGive(xM1);

		vTaskDelay(pdMS_TO_TICKS(1000));

	}
}

// task_2 takes M2 and then M1
/* Deadlock will happen if task 2 takes M2 and then M1, to prevent that we
 * following a consistent order similar to task 1*/
static void task_2(void *pvParams)
{
	vTaskDelay(pdMS_TO_TICKS(150)); // start after task 2
	uart_puts("Task 2: -> started \r\n");

	while(1)
	{
		uart_puts("Task 2: Taking the M1\r\n");
		xSemaphoreTake(xM1, portMAX_DELAY); // changed to M2 to M1
		uart_puts("Task 2: Got M1\r\n");

		// take M2
		uart_puts("Task 2: Taking M2\r\n");
		xSemaphoreTake(xM2, portMAX_DELAY); // deadlock will happen herr
		uart_puts("Task 2: Got M2\r\n");

		xSemaphoreGive(xM2);
		xSemaphoreGive(xM1);

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}


void vApplicationIdleHook(void)
{
	/* Reason to keep it empty instead of using _WFI(), because _WFI() causes the ST-Link
	 * debugger to lose its SWD connection which resets the board. In actual scenario
	 * the board will be used without debugger "upper part of the board" will be broken and
	 * _WFI() will be used to put the CPU into sleep mode & reduce the power consumption*/
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    uart_puts("\r\n*** STACK OVERFLOW: ");
    uart_puts(pcTaskName);
    uart_puts(" ***\r\n");
    taskDISABLE_INTERRUPTS();
    for (;;);
}

void vApplicationMallocFailedHook(void)
{
	uart_puts("\r\n*** MALLOC FAILED — increase configTOTAL_HEAP_SIZE ***\r\n");
	taskDISABLE_INTERRUPTS();
	for (;;);
}
/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();
  // Verify: this should print 180000000
  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  uart_puts("Starting\r\n");


  // Create kernel objects
  xM1 = xSemaphoreCreateMutex();
  xM2 = xSemaphoreCreateMutex();
  xUartMutex = xSemaphoreCreateMutex();

  if(!xM1 || !xM2 || !xUartMutex)
  {
	  uart_puts("FATAL: Unable to create kernel objects\r\n");
	  for (;;);
  }

  uart_puts("Creating tasks\r\n");
  BaseType_t r1 = xTaskCreate(task_1, "T1",  256, NULL, 1, NULL);
  BaseType_t r2 = xTaskCreate(task_2, "T2",  256, NULL, 1, NULL);
  if (r1 != pdPASS || r2 != pdPASS)
  {
	  uart_puts("FATAL: xTaskCreate failed — heap too small\r\n");
	  for (;;);
  }
  else
  {
	  uart_puts("Tasks created sucessfully\r\n");
  }

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  uart_puts("Tasks created. Starting scheduler...\r\n");
  vTaskStartScheduler();   /* Never returns if heap is sufficient */

  uart_puts("FATAL: scheduler returned — heap exhausted\r\n");
  for (;;);

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
