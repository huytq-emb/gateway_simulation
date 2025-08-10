/**
 ******************************************************************************
 * @file    uart_drv.h
 * @brief   UART driver header for STM32F407 Gateway ECU
 * @author  Automotive Firmware Engineer
 * @date    August 2025
 ******************************************************************************
 */

#ifndef UART_DRV_H
#define UART_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief UART error types
 */
typedef enum {
    UART_ERROR_NONE = 0,
    UART_ERROR_OVERRUN,
    UART_ERROR_FRAMING,
    UART_ERROR_PARITY,
    UART_ERROR_BUFFER_FULL
} UartError_t;

/* Exported constants --------------------------------------------------------*/
#define UART_TX_BUFFER_SIZE     256     /* TX ring buffer size */
#define UART_RX_BUFFER_SIZE     128     /* RX ring buffer size */

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
bool UART_Init(uint32_t baudrate);
bool UART_Write(const char* str);
bool UART_WriteData(const uint8_t* data, uint16_t length);
bool UART_Read(char* data, uint16_t* length);
uint16_t UART_GetTxFreeSpace(void);
uint16_t UART_GetRxCount(void);
UartError_t UART_GetLastError(void);
void UART_ClearError(void);
void UART_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_DRV_H */
