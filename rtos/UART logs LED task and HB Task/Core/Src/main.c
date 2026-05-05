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
static SemaphoreHandle_t xUartMutex = NULL;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);


static QueueHandle_t xRxQueue = NULL;


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

static void uart_print_u32(const char *ch, uint32_t value)
{
	char buffer[32];
	char temp[12];

	int i = 0, j = 0;

	// "ch" msg
	while(ch[i])
	{
		buffer[j++] = ch[i++];
	}

	// print the value
	if(value == 0)
	{
		buffer[j++] = '0'; // simple case
	}
	else // TODO: Need to revist the below logic
	{
		int k = 0;
		while (value > 0)
		{
			temp[k++] = '0' + (value % 10);
			value /= 10;
		}
		while (k > 0)
		{
			buffer[j++] = temp[--k];
		}
	}

	buffer[j++]='\r';
	buffer[j++]='\n';
	buffer[j] = 0;

	uart_puts(buffer);
}


/* A simple task for LED blink & heartbeat*/
static void task_led(void *pvParams)
{
	uart_puts("LED Task Started. Priority=1\r\n");
	//\n\r might give an empty line, so the standard is \r\n

	/* Add this temporarily — remove after debugging */
	    UBaseType_t wm = uxTaskGetStackHighWaterMark(NULL);
	    /* print wm using uart_print_u32 */
	    uart_print_u32("LED stack watermark=", wm);

	uint32_t count = 0;
	while(1)
	{
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
		/*Led ON, LD2 in pinout diagram in CUBE*/
		uart_puts("LED is ON\r\n");
		vTaskDelay(pdMS_TO_TICKS(500));
		/*always use ticks*/

		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
		/*LED off*/
		uart_puts("LED is off\r\n");
		vTaskDelay(pdMS_TO_TICKS(500));

		count++;
		uart_print_u32("LED cycle: ", count);
	}
}

/* A simple tcik counter*/
static void task_hearbeat(void *pvParams)
{
	 uart_puts("HB Task started. Priority=1\r\n");

	 while(1)
	 {
		 vTaskDelay(pdMS_TO_TICKS(2000));
		 uart_print_u32("HB tick=",xTaskGetTickCount());

		 // printing heap size
		 uart_print_u32("HB free_heap=", xPortGetFreeHeapSize());
	 }
}

/* A function to repricate the console characters*/
static void task_uart_rx(void *pvParams)
{
	uart_puts("RX Task started. Type characters\r\n");

	char ch;
	while(1)
	{
		// wait until a character is arrived in the queue
		if(xQueueReceive(xRxQueue, &ch, portMAX_DELAY) == pdPASS)
		{
			// form echo command with ch
			char echoCommand[8] = {'E', 'C', '>', ch, '\r', '\n', 0};
			uart_puts(echoCommand);

			if(ch == '\r')
			{
				uart_puts("Enter key is pressed\r\n");
			}
		}
	}
}

/*USART2 ISR -> this will replace any USART2_IRQHandler
 * Enable USART2 RXNE interrupt in MX_USART2_UART_Init*/
void USART2_IRQHandler(void)
{
	 if (USART2->SR & (1U << 5)) /* RXNE flag */
	 {
		 char c = (char)USART2->DR;                  /* clears RXNE */
		 BaseType_t xHigherPriorityTaskWoken  = pdFALSE; /*alawys to be initalized*/

		 xQueueSendFromISR(xRxQueue, &c, &xHigherPriorityTaskWoken);
		 portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	 }
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


  /* Enable USART2 RXNE interrupt for the RX task */
  USART2->CR1 |= (1U << 5);                       /* RXNEIE bit */
  NVIC_SetPriority(USART2_IRQn, 5);               /* >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY */
  NVIC_EnableIRQ(USART2_IRQn);

  uart_puts("Starting\r\n");

  /* USER CODE BEGIN RTOS_QUEUES */
  xRxQueue = xQueueCreate(32, sizeof(char));
  if (xRxQueue == NULL) {
	  uart_puts("FATAL: queue create failed\r\n");
	  for (;;);
  }
  /* USER CODE END RTOS_QUEUES */

  /* Create tasks — check return values! */


  xUartMutex = xSemaphoreCreateMutex();
  if(xUartMutex == NULL)
  {
	  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
	  for (;;);
  }

  uart_puts("Creating tasks\r\n");
  BaseType_t r1 = xTaskCreate(task_led,       "LED",  256, NULL, 1, NULL);
  BaseType_t r2 = xTaskCreate(task_hearbeat, "HB",   256, NULL, 1, NULL);
  BaseType_t r3 = xTaskCreate(task_uart_rx,   "RX",   256, NULL, 2, NULL);

  if (r1 != pdPASS || r2 != pdPASS || r3 != pdPASS)
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
