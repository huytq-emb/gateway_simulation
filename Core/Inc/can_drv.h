/**
 ******************************************************************************
 * @file    can_drv.h
 * @brief   CAN driver header for STM32F407 Gateway ECU
 * @author  Automotive Firmware Engineer
 * @date    August 2025
 ******************************************************************************
 */

#ifndef CAN_DRV_H
#define CAN_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief CAN frame structure
 */
typedef struct {
    uint32_t id;            /* CAN identifier */
    uint8_t dlc;            /* Data length code (0-8) */
    uint8_t data[8];        /* Data bytes */
    uint32_t timestamp;     /* Reception timestamp */
} CanFrame_t;

/**
 * @brief CAN error types
 */
typedef enum {
    CAN_ERROR_NONE = 0,
    CAN_ERROR_BUS_OFF,
    CAN_ERROR_ERROR_PASSIVE,
    CAN_ERROR_WARNING,
    CAN_ERROR_OVERRUN,
    CAN_ERROR_TIMEOUT
} CanError_t;

/* Exported constants --------------------------------------------------------*/
#define CAN_RX_BUFFER_SIZE      16      /* RX ring buffer size */
#define CAN_FILTER_ID_ENGINE    0x100   /* Engine RPM CAN ID */
#define CAN_FILTER_ID_TEMP      0x101   /* Engine temperature CAN ID */
#define CAN_FILTER_ID_SPEED     0x102   /* Vehicle speed CAN ID */

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
bool CAN_Init(uint32_t baudrate);
bool CAN_Send(uint32_t id, const uint8_t* data, uint8_t dlc);
bool CAN_Receive(CanFrame_t* frame);
uint16_t CAN_GetRxCount(void);
CanError_t CAN_GetLastError(void);
void CAN_ClearError(void);
void CAN_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* CAN_DRV_H */
