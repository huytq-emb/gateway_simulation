/**
 ******************************************************************************
 * @file    system_config.h
 * @brief   System configuration header for STM32F407 Gateway ECU
 * @author  Automotive Firmware Engineer
 * @date    August 2025
 ******************************************************************************
 */

#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define HSE_VALUE               8000000U    /* 8 MHz external crystal */
#define SYSTEM_CLOCK_FREQ       168000000U  /* 168 MHz system clock */
#define APB1_CLOCK_FREQ         42000000U   /* 42 MHz APB1 clock */
#define APB2_CLOCK_FREQ         84000000U   /* 84 MHz APB2 clock */

/* CAN Pin Configuration */
#define CAN1_RX_PIN             GPIO_PIN_11
#define CAN1_TX_PIN             GPIO_PIN_12
#define CAN1_GPIO_PORT          GPIOA
#define CAN1_GPIO_AF            GPIO_AF9_CAN1

/* UART Pin Configuration */
#define USART3_TX_PIN           GPIO_PIN_10
#define USART3_RX_PIN           GPIO_PIN_11
#define USART3_GPIO_PORT        GPIOB
#define USART3_GPIO_AF          GPIO_AF7_USART3

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void SystemConfig_Init(void);
void SystemClock_Config(void);
void GPIO_Config(void);
void NVIC_Config(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_CONFIG_H */
