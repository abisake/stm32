This is a small hands-on project on STM32 NUCLEO-F446RE board, both bare-metal programming i.e. directly accessing the registers with help reference manuals & datasheets and using FreeRTOS to understand the RTOS concepts.

Hardware used:
Board        STM32 Nucleo-F446RE (LQFP64, MB1136)
CPU          ARM Cortex-M4F @ 180 MHz
Clock        HSI 16 MHz → PLL (M=8, N=180, P=2) → 180 MHz SYSCLK
Flash        512 KB — 5 wait states at 180 MHz
SRAM         128 KB
Toolchain    STM32CubeIDE, arm-none-eabi-gcc 14.3, FreeRTOS

Bare metal Projects:
1. Led + UART poling   -> GPIO MODER/BSRR, USART2 BRR/CR1, bare-metal clock config
2. PLL Clock with 180 MHZ
3. UART using interrupt -> RXNE ISR
4. UART using simple DMA -> DMA1 Stream 6 Ch4, HISR/HIFCR, memory-to-peripheral transfer

FreeRTOS Project:
1. LED + Heartbeat + UART
2. Understanding Semaphore via button press
3. Understanding priority inversion and inheritance // In progress
