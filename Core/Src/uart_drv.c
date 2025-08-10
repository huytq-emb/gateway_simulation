/**
 ******************************************************************************
 * @file    uart_drv.c
 * @brief   UART driver implementation for STM32F407 Gateway ECU
 * @author  Automotive Firmware Engineer
 * @date    August 2025
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "uart_drv.h"
#include "system_config.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint8_t tx_buffer[UART_TX_BUFFER_SIZE];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;
static volatile uint16_t tx_count = 0;

static uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;
static volatile uint16_t rx_count = 0;

static volatile UartError_t last_error = UART_ERROR_NONE;
static volatile bool tx_in_progress = false;

/* Private function prototypes -----------------------------------------------*/
static void UART_StartTransmission(void);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  Initialize UART peripheral
 * @param  baudrate: UART baudrate (e.g., 115200)
 * @retval true if successful, false otherwise
 */
bool UART_Init(uint32_t baudrate)
{
    /* Enable USART3 clock */
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    
    /* Reset USART3 to ensure clean state */
    RCC->APB1RSTR |= RCC_APB1RSTR_USART3RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_USART3RST;
    
    /* Small delay after reset */
    for(volatile int i = 0; i < 1000; i++);
    
    /* Calculate baudrate register value with fractional part */
    /* BRR format: [15:4] = Mantissa, [3:0] = Fraction */
    /* BRR = APB1_FREQ / (16 * baudrate) - USART3 is on APB1! */
    
    uint32_t usartdiv = (APB1_CLOCK_FREQ * 25) / (4 * baudrate);  /* Multiply by 25 for precision */
    uint32_t mantissa = usartdiv / 100;                            /* Integer part */
    uint32_t fraction = ((usartdiv - (mantissa * 100)) * 16 + 50) / 100; /* Fractional part */
    
    /* Handle fraction overflow */
    if (fraction >= 16) {
        mantissa++;
        fraction = 0;
    }
    
    uint32_t brr = (mantissa << 4) | (fraction & 0x0F);
    
    /* Debug calculation for 115200 baud:
     * usartdiv = (42000000 * 25) / (4 * 115200) = 1050000000 / 460800 = 2278.6458
     * mantissa = 2278 / 100 = 22
     * fraction = ((2278 - 2200) * 16 + 50) / 100 = (78 * 16 + 50) / 100 = 1298 / 100 = 12
     * BRR = (22 << 4) | 12 = 0x160 | 0xC = 0x16C
     */
    
    USART3->BRR = brr;
    
    /* Configure UART parameters */
    USART3->CR1 = USART_CR1_UE |        /* USART enable */
                  USART_CR1_TE |        /* Transmitter enable */
                  USART_CR1_RE |        /* Receiver enable */
                  USART_CR1_RXNEIE;     /* RX not empty interrupt */
    
    USART3->CR2 = 0;                    /* 1 stop bit, no clock output */
    USART3->CR3 = 0;                    /* No hardware flow control */
    
    /* Wait for UART to be ready */
    while (!(USART3->SR & USART_SR_TC));
    
    /* Clear status register */
    USART3->SR = 0;
    
    /* Clear error flags */
    UART_ClearError();
    
    return true;
}

/**
 * @brief  Write string to UART (non-blocking)
 * @param  str: Null-terminated string to send
 * @retval true if successful, false if buffer full
 */
bool UART_Write(const char* str)
{
    if (str == NULL) return false;
    
    uint16_t length = strlen(str);
    return UART_WriteData((const uint8_t*)str, length);
}

/**
 * @brief  Write data to UART (non-blocking)
 * @param  data: Pointer to data buffer
 * @param  length: Number of bytes to send
 * @retval true if successful, false if buffer full
 */
bool UART_WriteData(const uint8_t* data, uint16_t length)
{
    if (data == NULL || length == 0) return false;
    
    /* Check if enough space in buffer */
    if ((UART_TX_BUFFER_SIZE - tx_count) < length) {
        last_error = UART_ERROR_BUFFER_FULL;
        return false;
    }
    
    /* Disable interrupts for atomic operation */
    __disable_irq();
    
    /* Copy data to buffer */
    for (uint16_t i = 0; i < length; i++) {
        tx_buffer[tx_head] = data[i];
        tx_head = (tx_head + 1) % UART_TX_BUFFER_SIZE;
        tx_count++;
    }
    
    /* Start transmission if not already in progress */
    if (!tx_in_progress) {
        UART_StartTransmission();
    }
    
    __enable_irq();
    
    return true;
}

/**
 * @brief  Read data from UART buffer
 * @param  data: Pointer to data buffer
 * @param  length: Pointer to length (input: max bytes, output: actual bytes)
 * @retval true if data available, false if buffer empty
 */
bool UART_Read(char* data, uint16_t* length)
{
    if (data == NULL || length == NULL || rx_count == 0) {
        if (length) *length = 0;
        return false;
    }
    
    /* Disable interrupts for atomic operation */
    __disable_irq();
    
    uint16_t bytes_to_read = (*length < rx_count) ? *length : rx_count;
    
    /* Copy data from buffer */
    for (uint16_t i = 0; i < bytes_to_read; i++) {
        data[i] = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % UART_RX_BUFFER_SIZE;
        rx_count--;
    }
    
    *length = bytes_to_read;
    
    __enable_irq();
    
    return true;
}

/**
 * @brief  Get free space in TX buffer
 * @retval Number of free bytes
 */
uint16_t UART_GetTxFreeSpace(void)
{
    return UART_TX_BUFFER_SIZE - tx_count;
}

/**
 * @brief  Get number of bytes in RX buffer
 * @retval Number of available bytes
 */
uint16_t UART_GetRxCount(void)
{
    return rx_count;
}

/**
 * @brief  Get last UART error
 * @retval Last error code
 */
UartError_t UART_GetLastError(void)
{
    return last_error;
}

/**
 * @brief  Clear UART error flags
 */
void UART_ClearError(void)
{
    last_error = UART_ERROR_NONE;
    /* Clear UART status register */
    (void)USART3->SR;
    (void)USART3->DR;
}

/**
 * @brief  UART interrupt handler
 */
void UART_IRQHandler(void)
{
    uint32_t sr = USART3->SR;
    
    /* Receive data register not empty */
    if (sr & USART_SR_RXNE) {
        uint8_t data = USART3->DR;
        
        /* Check for buffer overflow */
        if (rx_count < UART_RX_BUFFER_SIZE) {
            rx_buffer[rx_head] = data;
            rx_head = (rx_head + 1) % UART_RX_BUFFER_SIZE;
            rx_count++;
        } else {
            last_error = UART_ERROR_OVERRUN;
        }
    }
    
    /* Transmit data register empty */
    if ((sr & USART_SR_TXE) && (USART3->CR1 & USART_CR1_TXEIE)) {
        if (tx_count > 0) {
            /* Send next byte */
            USART3->DR = tx_buffer[tx_tail];
            tx_tail = (tx_tail + 1) % UART_TX_BUFFER_SIZE;
            tx_count--;
        } else {
            /* No more data to send, disable TXE interrupt */
            USART3->CR1 &= ~USART_CR1_TXEIE;
            tx_in_progress = false;
        }
    }
    
    /* Transmission complete */
    if (sr & USART_SR_TC) {
        USART3->SR &= ~USART_SR_TC; /* Clear TC flag */
    }
    
    /* Error handling */
    if (sr & USART_SR_ORE) {
        last_error = UART_ERROR_OVERRUN;
        (void)USART3->DR; /* Clear ORE flag */
    }
    
    if (sr & USART_SR_FE) {
        last_error = UART_ERROR_FRAMING;
        (void)USART3->DR; /* Clear FE flag */
    }
    
    if (sr & USART_SR_PE) {
        last_error = UART_ERROR_PARITY;
        (void)USART3->DR; /* Clear PE flag */
    }
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Start UART transmission
 */
static void UART_StartTransmission(void)
{
    if (tx_count > 0 && !tx_in_progress) {
        tx_in_progress = true;
        
        /* Send first byte */
        USART3->DR = tx_buffer[tx_tail];
        tx_tail = (tx_tail + 1) % UART_TX_BUFFER_SIZE;
        tx_count--;
        
        /* Enable TXE interrupt */
        USART3->CR1 |= USART_CR1_TXEIE;
    }
}
