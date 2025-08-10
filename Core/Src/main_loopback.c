/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main_loopback.c
  * @brief          : STM32F407 Gateway ECU Main Application - LOOPBACK MODE
  * @author         : Automotive Firmware Engineer  
  * @date           : August 2025
  * @version        : 1.0
  * @note           : This version runs CAN in loopback mode for testing
  *                   without external CAN hardware
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
#define TEST_FRAME_INTERVAL_MS  1000        /* Test frame generation interval */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint32_t last_stats_time = 0;
static uint32_t last_test_frame_time = 0;
static uint16_t test_rpm = 1000;
static uint8_t test_temp = 80;
static uint16_t test_speed = 50;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
static void Gateway_Init(void);
static void Gateway_ProcessCanMessages(void);
static void Gateway_PrintStatistics(void);
static void Gateway_SendTestFrames(void);
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
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  // HAL_Init();  // Removed - using MCAL

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  
  /* Initialize Gateway ECU components */
  Gateway_Init();
  
  /* Send simple test pattern first to verify UART */
  UART_Write("A\r\n");
  for(volatile int i = 0; i < 1000000; i++);  /* Simple delay */
  UART_Write("TEST\r\n");
  for(volatile int i = 0; i < 1000000; i++);  /* Simple delay */
  UART_Write("12345\r\n");
  for(volatile int i = 0; i < 1000000; i++);  /* Simple delay */
  
  /* Send startup message for loopback mode */
  UART_Write("Gateway ECU Started - LOOPBACK MODE\r\n");
  UART_Write("Generating test CAN frames internally\r\n");
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* Generate test CAN frames (loopback mode) */
    Gateway_SendTestFrames();
    
    /* Process CAN messages */
    Gateway_ProcessCanMessages();
    
    /* Router polling for error handling */
    Router_Poll();
    
    /* Print statistics periodically */
    Gateway_PrintStatistics();
    
    /* Small delay to prevent excessive CPU usage */
    /* Simple delay loop */
    for(volatile int i = 0; i < 168000; i++);  /* ~1ms delay at 168MHz */
    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
//void SystemClock_Config(void)
//{
//  /* This function is called by HAL_Init() but we use our own MCAL implementation */
//  /* The actual clock configuration is done in SystemConfig_Init() */
//}

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
 * @brief  Initialize Gateway ECU components (LOOPBACK MODE)
 * @param  None
 * @retval None
 */
static void Gateway_Init(void)
{
  /* Initialize system configuration (clocks, GPIO, NVIC) */
  SystemConfig_Init();
  
  /* Enable CAN1 clock for loopback configuration */
  RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
  
  /* Configure CAN1 in loopback mode */
  /* Request initialization mode */
  CAN1->MCR |= CAN_MCR_INRQ;
  
  /* Wait for initialization mode acknowledgment */
  uint32_t timeout = SystemCoreClock / 1000; /* 1 ms timeout */
  while (!(CAN1->MSR & CAN_MSR_INAK) && timeout--);
  
  /* Configure CAN options with loopback mode enabled */
  CAN1->MCR = CAN_MCR_INRQ |         /* Initialization request */
              CAN_MCR_NART |          /* No automatic retransmission */
              CAN_MCR_AWUM |          /* Automatic wake-up mode */
              CAN_MCR_ABOM;           /* Automatic bus-off management */
  
  /* Configure bit timing for 500 kbit/s */
  /* Prescaler = 6 (5+1), BS1 = 13 tq (12+1), BS2 = 2 tq (1+1), SJW = 1 tq (0+1) */
  CAN1->BTR = (0 << CAN_BTR_SJW_Pos) |     /* SJW = 1 tq */
              (12 << CAN_BTR_TS1_Pos) |     /* BS1 = 13 tq */
              (1 << CAN_BTR_TS2_Pos) |      /* BS2 = 2 tq */
              (5) |                         /* Prescaler = 6 */
              CAN_BTR_LBKM;                 /* LOOPBACK MODE ENABLED */
  
  /* Configure receive filters */
  /* Enter filter initialization mode */
  CAN1->FMR |= CAN_FMR_FINIT;
  
  /* Configure filter 0 for all test IDs (0x100, 0x101, 0x102) */
  CAN1->FM1R &= ~CAN_FM1R_FBM0;      /* Identifier mask mode */
  CAN1->FS1R |= CAN_FS1R_FSC0;       /* 32-bit scale */
  CAN1->FFA1R &= ~CAN_FFA1R_FFA0;    /* FIFO 0 assignment */
  
  /* Set filter to accept IDs 0x100-0x107 */
  CAN1->sFilterRegister[0].FR1 = (0x100 << 21);
  CAN1->sFilterRegister[0].FR2 = (0x7F8 << 21); /* Mask for 0x100-0x107 */
  
  /* Activate filter 0 */
  CAN1->FA1R |= CAN_FA1R_FACT0;
  
  /* Leave filter initialization mode */
  CAN1->FMR &= ~CAN_FMR_FINIT;
  
  /* Enable FIFO 0 message pending interrupt */
  CAN1->IER = CAN_IER_FMPIE0 |        /* FIFO 0 message pending */
              CAN_IER_FOVIE0 |        /* FIFO 0 overrun */
              CAN_IER_BOFIE |         /* Bus-off */
              CAN_IER_EPVIE |         /* Error passive */
              CAN_IER_EWGIE;          /* Error warning */
  
  /* Leave initialization mode */
  CAN1->MCR &= ~CAN_MCR_INRQ;
  
  /* Wait for normal mode */
  timeout = SystemCoreClock / 1000;
  while ((CAN1->MSR & CAN_MSR_INAK) && timeout--);
  
  /* Initialize UART driver */
  if (!UART_Init(UART_BAUDRATE)) {
    Error_Handler();
  }
  
  /* Initialize PDU Router */
  Router_Init();
  
  /* Record initialization time */
  last_stats_time = HAL_GetTick();
  last_test_frame_time = HAL_GetTick();
}

/**
 * @brief  Send test CAN frames in loopback mode
 * @param  None
 * @retval None
 */
static void Gateway_SendTestFrames(void)
{
  static uint32_t frame_counter = 0;
  
  /* Send test frames every 1000 main loop iterations (~1 second at 1ms loop) */
  frame_counter++;
  if (frame_counter >= TEST_FRAME_INTERVAL_MS) {
    frame_counter = 0;  /* Reset counter */
    
    /* Debug message */
    UART_Write("Sending CAN test frame\r\n");
    
    /* Wait for available transmit mailbox */
    uint32_t timeout = 1000;
    while (!(CAN1->TSR & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2)) && timeout--);
    
    if (timeout > 0) {
      uint8_t mailbox = 0;
      if (CAN1->TSR & CAN_TSR_TME0) mailbox = 0;
      else if (CAN1->TSR & CAN_TSR_TME1) mailbox = 1;
      else if (CAN1->TSR & CAN_TSR_TME2) mailbox = 2;
      
      /* Send Engine RPM frame (ID 0x100) */
      uint16_t rpm_raw = test_rpm * 4;
      CAN1->sTxMailBox[mailbox].TIR = (0x100 << CAN_TI0R_STID_Pos);
      CAN1->sTxMailBox[mailbox].TDTR = 8; /* DLC = 8 */
      CAN1->sTxMailBox[mailbox].TDLR = (rpm_raw & 0xFF) | ((rpm_raw >> 8) << 8);
      CAN1->sTxMailBox[mailbox].TDHR = 0;
      CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ;
      
      /* Small delay between frames */
      for(volatile int i = 0; i < 1680000; i++);  /* ~10ms delay */
      
      /* Wait for next mailbox */
      timeout = 1000;
      while (!(CAN1->TSR & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2)) && timeout--);
      
      if (timeout > 0) {
        if (CAN1->TSR & CAN_TSR_TME0) mailbox = 0;
        else if (CAN1->TSR & CAN_TSR_TME1) mailbox = 1;
        else if (CAN1->TSR & CAN_TSR_TME2) mailbox = 2;
        
        /* Send Engine Temperature frame (ID 0x101) */
        uint8_t temp_raw = test_temp + 40;
        CAN1->sTxMailBox[mailbox].TIR = (0x101 << CAN_TI0R_STID_Pos);
        CAN1->sTxMailBox[mailbox].TDTR = 8; /* DLC = 8 */
        CAN1->sTxMailBox[mailbox].TDLR = temp_raw << 16; /* Byte 2 */
        CAN1->sTxMailBox[mailbox].TDHR = 0;
        CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ;
        
        for(volatile int i = 0; i < 1680000; i++);  /* ~10ms delay */
        
        /* Wait for next mailbox */
        timeout = 1000;
        while (!(CAN1->TSR & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2)) && timeout--);
        
        if (timeout > 0) {
          if (CAN1->TSR & CAN_TSR_TME0) mailbox = 0;
          else if (CAN1->TSR & CAN_TSR_TME1) mailbox = 1;
          else if (CAN1->TSR & CAN_TSR_TME2) mailbox = 2;
          
          /* Send Vehicle Speed frame (ID 0x102) */
          uint16_t speed_raw = test_speed * 10;
          CAN1->sTxMailBox[mailbox].TIR = (0x102 << CAN_TI0R_STID_Pos);
          CAN1->sTxMailBox[mailbox].TDTR = 8; /* DLC = 8 */
          CAN1->sTxMailBox[mailbox].TDLR = 0;
          CAN1->sTxMailBox[mailbox].TDHR = (speed_raw & 0xFF) | ((speed_raw >> 8) << 8); /* Bytes 4-5 */
          CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ;
        }
      }
    }
    
    /* Update test values for next iteration */
    test_rpm += 100;
    if (test_rpm > 6000) test_rpm = 1000;
    
    test_temp += 5;
    if (test_temp > 110) test_temp = 80;
    
    test_speed += 10;
    if (test_speed > 120) test_speed = 50;
    
    /* Debug message */
    UART_Write("Test frame sent, values updated\r\n");
  }
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
  static uint32_t stats_counter = 0;
  
  /* Check if it's time to print statistics (every 10 seconds) */
  stats_counter++;
  if (stats_counter >= STATS_PRINT_INTERVAL_MS) {
    stats_counter = 0;  /* Reset counter */
    
    RouterStats_t stats;
    Router_GetStatistics(&stats);
    
    /* Format and send statistics */
    char stats_msg[128];
    sprintf(stats_msg, "STATS,Processed:%lu,Routed:%lu,Dropped:%lu,CANErr:%lu,UARTErr:%lu\r\n",
            stats.frames_processed, stats.frames_routed, stats.frames_dropped,
            stats.can_errors, stats.uart_errors);
    
    UART_Write(stats_msg);
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
