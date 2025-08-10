/**
 ******************************************************************************
 * @file    system_config.c
 * @brief   System configuration implementation for STM32F407 Gateway ECU
 * @author  Automotive Firmware Engineer
 * @date    August 2025
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "system_config.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  Initialize system configuration
 * @param  None
 * @retval None
 */
void SystemConfig_Init(void)
{
    /* Configure system clock to 168 MHz */
    SystemClock_Config();
    
    /* Configure GPIO for CAN and UART */
    GPIO_Config();
    
    /* Configure NVIC priorities */
    NVIC_Config();
}

/**
 * @brief  System Clock Configuration
 *         System Clock source = PLL (HSE)
 *         SYSCLK(Hz) = 168000000
 *         HCLK(Hz) = 168000000
 *         AHB Prescaler = 1
 *         APB1 Prescaler = 4 (APB1 = 42 MHz)
 *         APB2 Prescaler = 2 (APB2 = 84 MHz)
 * @param  None
 * @retval None
 */
void SystemClock_Config(void)
{
    /* Enable HSE oscillator */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));
    
    /* Configure Flash latency for 168 MHz operation */
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_5WS;
    
    /* Configure PLL: HSE * (N/M) / P = 8 * (336/8) / 2 = 168 MHz */
    RCC->PLLCFGR = (8 << RCC_PLLCFGR_PLLM_Pos) |      /* M = 8 */
                   (336 << RCC_PLLCFGR_PLLN_Pos) |     /* N = 336 */
                   (0 << RCC_PLLCFGR_PLLP_Pos) |       /* P = 2 */
                   (7 << RCC_PLLCFGR_PLLQ_Pos) |       /* Q = 7 */
                   RCC_PLLCFGR_PLLSRC_HSE;              /* HSE as source */
    
    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));
    
    /* Configure AHB, APB1, APB2 prescalers */
    RCC->CFGR = RCC_CFGR_HPRE_DIV1 |        /* AHB = SYSCLK/1 = 168 MHz */
                RCC_CFGR_PPRE1_DIV4 |        /* APB1 = HCLK/4 = 42 MHz */
                RCC_CFGR_PPRE2_DIV2;         /* APB2 = HCLK/2 = 84 MHz */
    
    /* Select PLL as system clock */
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
    
    /* Update SystemCoreClock variable */
    SystemCoreClock = SYSTEM_CLOCK_FREQ;
}

/**
 * @brief  Configure GPIO pins for CAN and UART
 * @param  None
 * @retval None
 */
void GPIO_Config(void)
{
    /* Enable GPIO clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    
    /* Configure CAN1 pins (PA11, PA12) */
    /* PA11 - CAN1_RX: Alternate Function, Pull-up */
    GPIOA->MODER &= ~(GPIO_MODER_MODE11);
    GPIOA->MODER |= GPIO_MODER_MODE11_1;        /* Alternate function */
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD11);
    GPIOA->PUPDR |= GPIO_PUPDR_PUPD11_0;        /* Pull-up */
    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL11);
    GPIOA->AFR[1] |= (GPIO_AF9_CAN1 << GPIO_AFRH_AFSEL11_Pos);
    
    /* PA12 - CAN1_TX: Alternate Function, Push-pull */
    GPIOA->MODER &= ~(GPIO_MODER_MODE12);
    GPIOA->MODER |= GPIO_MODER_MODE12_1;        /* Alternate function */
    GPIOA->OTYPER &= ~GPIO_OTYPER_OT12;         /* Push-pull */
    GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED12;    /* High speed */
    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL12);
    GPIOA->AFR[1] |= (GPIO_AF9_CAN1 << GPIO_AFRH_AFSEL12_Pos);
    
    /* Configure USART3 pins (PB10, PB11) */
    /* PB10 - USART3_TX: Alternate Function, Push-pull */
    GPIOB->MODER &= ~(GPIO_MODER_MODE10);
    GPIOB->MODER |= GPIO_MODER_MODE10_1;        /* Alternate function */
    GPIOB->OTYPER &= ~GPIO_OTYPER_OT10;         /* Push-pull */
    GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEED10;    /* High speed */
    GPIOB->AFR[1] &= ~(GPIO_AFRH_AFSEL10);
    GPIOB->AFR[1] |= (GPIO_AF7_USART3 << GPIO_AFRH_AFSEL10_Pos);
    
    /* PB11 - USART3_RX: Alternate Function, Pull-up */
    GPIOB->MODER &= ~(GPIO_MODER_MODE11);
    GPIOB->MODER |= GPIO_MODER_MODE11_1;        /* Alternate function */
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD11);
    GPIOB->PUPDR |= GPIO_PUPDR_PUPD11_0;        /* Pull-up */
    GPIOB->AFR[1] &= ~(GPIO_AFRH_AFSEL11);
    GPIOB->AFR[1] |= (GPIO_AF7_USART3 << GPIO_AFRH_AFSEL11_Pos);
}

/**
 * @brief  Configure NVIC priorities
 * @param  None
 * @retval None
 */
void NVIC_Config(void)
{
    /* Set priority group to 4 bits for preemption priority */
    NVIC_SetPriorityGrouping(0x03);
    
    /* Configure CAN1 RX0 interrupt priority */
    NVIC_SetPriority(CAN1_RX0_IRQn, NVIC_EncodePriority(0x03, 1, 0));
    NVIC_EnableIRQ(CAN1_RX0_IRQn);
    
    /* Configure USART3 interrupt priority */
    NVIC_SetPriority(USART3_IRQn, NVIC_EncodePriority(0x03, 2, 0));
    NVIC_EnableIRQ(USART3_IRQn);
}
