#include "uart.h"

#define RX_BUF_SIZE  64U
#define RX_BUF_MASK  (RX_BUF_SIZE - 1U)
static volatile char rx_buffer[RX_BUF_SIZE];
static volatile uint8_t rx_write = 0;
static uint8_t rx_read  = 0;

static volatile bool dma_tx_busy = false;

// For debugging in live expressions
volatile uint32_t usart2_isr_count = 0;
volatile uint32_t dma_isr_count = 0;
// Unablr to see the values, need to investigate

// uart init
void uart_init(void)
{
    RCC->AHB1ENR |= (1U << 0);   // GPIOA clock (page 143)
    RCC->APB1ENR |= (1U << 17);  // USART2 clock (page 145)

    GPIOA->MODER &= ~(0x3U << 4);
    GPIOA->MODER |= (0x2U << 4);  // PA2 AF

    GPIOA->MODER &= ~(0x3U << 6);
    GPIOA->MODER |= (0x2U << 6);  // PA3 AF

    GPIOA->AFR[0] &= ~(0xFU << 8);
    GPIOA->AFR[0] |= (0x7U << 8);  // PA2=AF7

    GPIOA->AFR[0] &= ~(0xFU << 12);
    GPIOA->AFR[0] |= (0x7U << 12); // PA3=AF7

    USART2->BRR = 0x008B;  // 115200 at 16 MHz HSI
    USART2->CR1 = (1U<<5)|(1U<<3)|(1U<<2)|(1U<<13); // RXNEIE|TE|RE|UE

    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_SetPriority(USART2_IRQn, 1);
}

void uart_dma_init(void)
{
	//Enable DMA1 clock (AHB! bus -> DMA1EN, 21 bit) P.143
	RCC->AHB1ENR |= (1U << 21);

	// Enable USART2 DMA TX (USART COntrol Register
	USART2->CR3 |= (1U << 7);

	// Enable DMA1 Stream6 (Table 28)
	NVIC_EnableIRQ(DMA1_Stream6_IRQn);
	NVIC_SetPriority(DMA1_Stream6_IRQn, 2); // lower priority than UART RX
}

void uart_dma_send(const uint8_t *buf, uint16_t len)
{
	if (dma_tx_busy) return; // previous transfer is on-going

	dma_tx_busy=true;

	// Configure DMA1 Stream 6
	DMA1_Stream6->CR = 0; // initially disable the stream
	while(DMA1_Stream6->CR & 1); // check EN bit is cleared or not

	DMA1->HIFCR = 0x3FU << 16;	// Clear HICFR bits[21:16], Section 9.5.4, P.224

	DMA1_Stream6->PAR = (uint32_t)&USART2->DR;	// peripheral address register
	DMA1_Stream6->M0AR = (uint32_t)buf;			// memory 0 address register
	DMA1_Stream6->NDTR = len;					// number of data items to transfer register

	DMA1_Stream6->CR =
			(4U << 25) |   // CHSEL bits[27:25] = 4 → Channel 4 = USART2_TX (Table 28)
			(1U << 16) |   // TCIE: transfer complete interrupt enable
			(0U << 13) |   // MSIZE = 00: memory data size = byte
			(0U << 11) |   // PSIZE = 00: peripheral data size = byte
			(1U << 10) |   // MINC = 1: increment memory address each transfer
			(0U << 9)  |   // PINC = 0: don't increment peripheral address (always DR)
			(1U << 6)  |   // DIR = 01: memory-to-peripheral direction
			(1U << 0);     // EN: enable stream — starts transfer immediately
}

// ISR: fires when all bytes have been transferred
void DMA1_Stream6_IRQHandler(void)
{
	dma_isr_count++;
	if(DMA1->HISR & (1U << 21))   // TC1F6: stream6's flag to check transfer is complete or not
	{
		DMA1->HIFCR = (1U << 21);	// clear the flag
		dma_tx_busy = false;
	}
}

void USART2_IRQHandler(void)
{
	usart2_isr_count++;
	if(USART2->SR & (1U << 5))
	{
		char ch = (char)USART2->DR;
		uint8_t next_write = (rx_write + 1U) & RX_BUF_MASK;
		if (next_write != rx_read)
		{
			rx_buffer[rx_write] = ch;
			rx_write = next_write;
		}
	}
}






