#include "uart.h"

// define the ring buffer
#define RX_BUF_SIZE 64U
#define RX_BUF_MASK (RX_BUF_SIZE - 1U)

static volatile char rx_buffer[RX_BUF_SIZE];
static volatile uint8_t rx_write = 0; 	// only written by ISR, that is why it is volatile
static uint8_t rx_read = 0;				// only written by main loop, so need for volatile
/* rx_write is volatile (ISR writes it, main reads it)
 * rx_read is NOT volatile (only main touches it)
 * */

// Configure UART
void uart_init(void)
{
	// ENable clocks
	RCC->AHB1ENR |= (1U << 0);		// GPIOA, P.143
	RCC->APB1ENR |= (1U << 17);		// USART2, P.145

	// Alternate functions PA2, PA3 -> P.185
	GPIOA->MODER &= ~(0x3U << 4);
	GPIOA->MODER |= (0x2U << 4);

	GPIOA->MODER &= ~(0x3U << 6);
	GPIOA->MODER |= (0x2U << 6);

	// OSPEEDR: high speed foe TX -> P.186
	GPIOA->OSPEEDR |= (0x3U << 4);

	// Configure AF7, (afrl -> P.187, table 11 datasheet)
	GPIOA->AFR[0] &= ~(0xFU << 8);
	GPIOA->AFR[0] |= (0x7U << 8);
	GPIOA->AFR[0] &= ~(0xFU << 12);
	GPIOA->AFR[0] |= (0x7U << 12);

	// BRR: at 16 MHz HSI, 115200 baud (p.817)
	USART2->BRR = 0x008B;

	// Enable the Read Data Register Not Empty(RXNE) interrupt in USART2's Control Register
	USART2->CR1 = (1U << 5)		// RXNE interrupt enable
			| (1U << 3)			// enable TX -> TE
			| (1U << 2)			// enable RX -> RE
			| (1U << 13);		// enable USART

	// Enable USARt2 Interupt in NVIC (Nested Vector Interrupt COntrolled)
	// Need to study about NVIC -> Page 235, Table 38
	NVIC_EnableIRQ(USART2_IRQn);
	NVIC_SetPriority(USART2_IRQn, 1);

}

// an ISR function which will be called automatically by hardware
void USART2_IRQHandler(void)
{
	// Check RXNE flag, in bit 5 -> P. 814
	if(USART2->SR & (1U << 5))
	{
		char byte = (char)USART2->DR;
		/* An interrupt is generated if RXNEIE=1 in the USART_CR1
register. It is cleared by a read to the USART_DR register */

		uint8_t next_write = (rx_write + 1U) & RX_BUF_MASK;
		// check buffer
		if (next_write != rx_read)
		{
			rx_buffer[rx_write] = byte;
			rx_write = next_write;
		}
	}
}

// simple polling TX,
void uart_putchar(char c)
{
	//	USART2 Status Register P.814
	while(!(USART2->SR & (1U << 7)));
	USART2->DR = (uint8_t)c;
}

void uart_puts(const char *s)
{
	while (*s) uart_putchar(*s++);
}

// read from buffer
bool uart_rx_available(void)
{
	// if pointers differs, data is present
	return rx_read != rx_write;
}

char uart_rx_read(void)
{
	if (!uart_rx_available()) return 0;

	char c = rx_buffer[rx_read];
	rx_read = (rx_read +1U) & RX_BUF_MASK;

	return c;
}







