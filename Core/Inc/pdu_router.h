/**
 ******************************************************************************
 * @file    pdu_router.h
 * @brief   PDU Router header for STM32F407 Gateway ECU
 * @author  Automotive Firmware Engineer
 * @date    August 2025
 ******************************************************************************
 */

#ifndef PDU_ROUTER_H
#define PDU_ROUTER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "can_drv.h"
#include "uart_drv.h"
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Signal extraction configuration
 */
typedef struct {
    uint32_t can_id;            /* CAN identifier */
    uint8_t start_byte;         /* Starting byte position in CAN data */
    uint8_t length;             /* Signal length in bytes (1, 2, or 4) */
    float scale;                /* Scaling factor */
    float offset;               /* Offset value */
    const char* format_string;  /* Printf format string for UART output */
    const char* signal_name;    /* Signal name for debugging */
} SignalConfig_t;

/**
 * @brief Router statistics
 */
typedef struct {
    uint32_t frames_processed;
    uint32_t frames_routed;
    uint32_t frames_dropped;
    uint32_t uart_errors;
    uint32_t can_errors;
} RouterStats_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void Router_Init(void);
void Router_ProcessCanFrame(const CanFrame_t* frame);
void Router_Poll(void);
void Router_GetStatistics(RouterStats_t* stats);
void Router_ClearStatistics(void);

#ifdef __cplusplus
}
#endif

#endif /* PDU_ROUTER_H */
