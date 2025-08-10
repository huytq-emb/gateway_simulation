/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : STM32F407 Gateway ECU Main Application
  * @author         : Automotive Firmware Engineer  
  * @date           : August 2025
  * @version        : 1.0
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "system_config.h"
#include "can_drv.h"
#include "uart_drv.h"
#include "pdu_router.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CAN_BAUDRATE            500000      /* 500 kbit/s */
#define UART_BAUDRATE           115200      /* 115200 baud */
#define MAIN_LOOP_DELAY_MS      1           /* Main loop delay */
#define STATS_PRINT_INTERVAL_MS 10000       /* Statistics print interval */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint32_t last_stats_time = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
static void Gateway_Init(void);
static void Gateway_ProcessCanMessages(void);
static void Gateway_PrintStatistics(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

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

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  
  /* Initialize Gateway ECU components */
  Gateway_Init();
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* Process CAN messages */
    Gateway_ProcessCanMessages();
    
    /* Router polling for error handling */
    Router_Poll();
    
    /* Print statistics periodically */
    Gateway_PrintStatistics();
    
    /* Small delay to prevent excessive CPU usage */
    HAL_Delay(MAIN_LOOP_DELAY_MS);
    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  /* This function is called by HAL_Init() but we use our own MCAL implementation */
  /* The actual clock configuration is done in SystemConfig_Init() */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO initialization is handled by our MCAL system_config module */
  /* This function is kept for HAL compatibility but functionality moved to GPIO_Config() */

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
 * @brief  Initialize Gateway ECU components
 * @param  None
 * @retval None
 */
static void Gateway_Init(void)
{
  /* Initialize system configuration (clocks, GPIO, NVIC) */
  SystemConfig_Init();
  
  /* Initialize CAN driver */
  if (!CAN_Init(CAN_BAUDRATE)) {
    Error_Handler();
  }
  
  /* Initialize UART driver */
  if (!UART_Init(UART_BAUDRATE)) {
    Error_Handler();
  }
  
  /* Initialize PDU Router */
  Router_Init();
  
  /* Record initialization time */
  last_stats_time = HAL_GetTick();
}

/**
 * @brief  Process incoming CAN messages
 * @param  None
 * @retval None
 */
static void Gateway_ProcessCanMessages(void)
{
  CanFrame_t frame;
  
  /* Process all available CAN frames */
  while (CAN_Receive(&frame)) {
    Router_ProcessCanFrame(&frame);
  }
}

/**
 * @brief  Print statistics periodically
 * @param  None
 * @retval None
 */
static void Gateway_PrintStatistics(void)
{
  uint32_t current_time = HAL_GetTick();
  
  /* Check if it's time to print statistics */
  if ((current_time - last_stats_time) >= STATS_PRINT_INTERVAL_MS) {
    RouterStats_t stats;
    Router_GetStatistics(&stats);
    
    /* Format and send statistics */
    char stats_msg[128];
    sprintf(stats_msg, "STATS,Processed:%lu,Routed:%lu,Dropped:%lu,CANErr:%lu,UARTErr:%lu\r\n",
            stats.frames_processed, stats.frames_routed, stats.frames_dropped,
            stats.can_errors, stats.uart_errors);
    
    UART_Write(stats_msg);
    
    last_stats_time = current_time;
  }
}

/* USER CODE END 4 */

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

#ifdef  USE_FULL_ASSERT
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
