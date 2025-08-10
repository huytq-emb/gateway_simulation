/**
 ******************************************************************************
 * @file    uart_test.c
 * @brief   Simple UART test to debug communication issues
 * @author  Debug Test
 * @date    August 2025
 ******************************************************************************
 */

#include "stm32f4xx.h"
#include "system_config.h"

/* Simple delay function */
static void delay_ms(uint32_t ms)
{
    for(uint32_t i = 0; i < ms; i++) {
        for(volatile uint32_t j = 0; j < 21000; j++);  /* ~1ms at 168MHz */
    }
}

/* Basic UART init with manual register setup */
void test_uart_init(void)
{
    /* Enable GPIOB and USART3 clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    
    /* Reset USART3 */
    RCC->APB1RSTR |= RCC_APB1RSTR_USART3RST;
    delay_ms(1);
    RCC->APB1RSTR &= ~RCC_APB1RSTR_USART3RST;
    delay_ms(1);
    
    /* Configure PB10 (TX) and PB11 (RX) as AF7 */
    GPIOB->MODER &= ~(GPIO_MODER_MODE10 | GPIO_MODER_MODE11);
    GPIOB->MODER |= (GPIO_MODER_MODE10_1 | GPIO_MODER_MODE11_1);  /* AF mode */
    
    GPIOB->OTYPER &= ~(GPIO_OTYPER_OT10 | GPIO_OTYPER_OT11);      /* Push-pull */
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED10 | GPIO_OSPEEDR_OSPEED11); /* High speed */
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD10 | GPIO_PUPDR_PUPD11);
    GPIOB->PUPDR |= GPIO_PUPDR_PUPD11_0;  /* Pull-up on RX */
    
    /* Set AF7 for USART3 */
    GPIOB->AFR[1] &= ~(GPIO_AFRH_AFSEL10 | GPIO_AFRH_AFSEL11);
    GPIOB->AFR[1] |= (0x07 << GPIO_AFRH_AFSEL10_Pos) | (0x07 << GPIO_AFRH_AFSEL11_Pos);
    
    /* Configure USART3 for 115200 baud */
    /* APB1 = 42MHz, USART3 is on APB1 */
    /* BRR = 42000000 / (16 * 115200) = 22.786... */
    /* Use precise calculation: */
    /* USARTDIV = 42000000 / 115200 = 364.583... */
    /* Mantissa = 22, Fraction = 12 (0.583 * 16 = 9.33 ≈ 9) */
    /* Actually: 22.786 -> Mantissa=22, Fraction=0.786*16=12.58≈13 */
    USART3->BRR = (22 << 4) | 13;  /* 0x16D */
    
    /* Configure USART3: 8N1, no flow control */
    USART3->CR1 = 0;
    USART3->CR2 = 0;
    USART3->CR3 = 0;
    
    /* Enable USART3 */
    USART3->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    
    /* Wait for USART to be ready */
    delay_ms(10);
}

/* Send single character */
void test_uart_send_char(char c)
{
    /* Wait for TXE */
    while (!(USART3->SR & USART_SR_TXE));
    USART3->DR = c;
    /* Wait for TC */
    while (!(USART3->SR & USART_SR_TC));
}

/* Send string */
void test_uart_send_string(const char* str)
{
    while (*str) {
        test_uart_send_char(*str++);
    }
}

int main(void)
{
    /* Basic system setup */
    SystemClock_Config();
    
    /* Initialize test UART */
    test_uart_init();
    
    /* Send test pattern */
    delay_ms(100);
    
    test_uart_send_string("UART Test Start\r\n");
    delay_ms(100);
    
    test_uart_send_string("ASCII: ");
    for (char c = 'A'; c <= 'Z'; c++) {
        test_uart_send_char(c);
    }
    test_uart_send_string("\r\n");
    delay_ms(100);
    
    test_uart_send_string("Numbers: ");
    for (char c = '0'; c <= '9'; c++) {
        test_uart_send_char(c);
    }
    test_uart_send_string("\r\n");
    delay_ms(100);
    
    /* Infinite loop with periodic test */
    while (1) {
        test_uart_send_string("Test Message\r\n");
        delay_ms(1000);
    }
}
