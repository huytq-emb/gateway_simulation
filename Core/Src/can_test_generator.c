/**
 ******************************************************************************
 * @file    can_test_generator.c
 * @brief   CAN test frame generator for Gateway ECU testing
 * @author  Automotive Firmware Engineer
 * @date    August 2025
 ******************************************************************************
 * @note    This file can be compiled separately for a second STM32 board
 *          to generate test CAN frames for the Gateway ECU
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "can_drv.h"
#include <stdint.h>

/* Private define ------------------------------------------------------------*/
#define TEST_FRAME_INTERVAL_MS  100     /* Send test frame every 100ms */

/* Private variables ---------------------------------------------------------*/
static uint32_t last_frame_time = 0;
static uint16_t rpm_value = 800;        /* Starting RPM value */
static uint8_t temp_value = 70;         /* Starting temperature (30°C = 70 raw) */
static uint16_t speed_value = 0;        /* Starting speed */

/* Test frame generation functions -------------------------------------------*/

/**
 * @brief  Generate Engine RPM test frame (CAN ID 0x100)
 * @param  None
 * @retval None
 */
void GenerateEngineRpmFrame(void)
{
    uint8_t data[8] = {0};
    
    /* Engine RPM: bytes 0-1, scale factor /4 */
    /* For 2000 RPM: raw_value = 2000 * 4 = 8000 = 0x1F40 */
    uint16_t raw_rpm = rpm_value * 4;
    data[0] = raw_rpm & 0xFF;           /* Low byte */
    data[1] = (raw_rpm >> 8) & 0xFF;    /* High byte */
    
    /* Send CAN frame */
    CAN_Send(0x100, data, 8);
    
    /* Update RPM for next frame (simulate engine acceleration) */
    rpm_value += 50;
    if (rpm_value > 6000) rpm_value = 800;
}

/**
 * @brief  Generate Engine Temperature test frame (CAN ID 0x101)
 * @param  None
 * @retval None
 */
void GenerateEngineTempFrame(void)
{
    uint8_t data[8] = {0};
    
    /* Engine Temperature: byte 2, offset +40°C */
    /* For 90°C: raw_value = 90 + 40 = 130 */
    data[2] = temp_value;
    
    /* Send CAN frame */
    CAN_Send(0x101, data, 8);
    
    /* Update temperature for next frame */
    temp_value++;
    if (temp_value > 130) temp_value = 70; /* 30°C to 90°C range */
}

/**
 * @brief  Generate Vehicle Speed test frame (CAN ID 0x102)
 * @param  None
 * @retval None
 */
void GenerateVehicleSpeedFrame(void)
{
    uint8_t data[8] = {0};
    
    /* Vehicle Speed: bytes 4-5, scale factor /10 */
    /* For 120 km/h: raw_value = 120 * 10 = 1200 = 0x04B0 */
    uint16_t raw_speed = speed_value * 10;
    data[4] = raw_speed & 0xFF;         /* Low byte */
    data[5] = (raw_speed >> 8) & 0xFF;  /* High byte */
    
    /* Send CAN frame */
    CAN_Send(0x102, data, 8);
    
    /* Update speed for next frame */
    speed_value += 5;
    if (speed_value > 120) speed_value = 0;
}

/**
 * @brief  Main test generator function
 * @param  None
 * @retval None
 */
void CANTestGenerator_Run(void)
{
    uint32_t current_time = HAL_GetTick();
    
    /* Generate test frames at specified interval */
    if ((current_time - last_frame_time) >= TEST_FRAME_INTERVAL_MS) {
        /* Generate all test frames */
        GenerateEngineRpmFrame();
        HAL_Delay(10);
        
        GenerateEngineTempFrame();
        HAL_Delay(10);
        
        GenerateVehicleSpeedFrame();
        HAL_Delay(10);
        
        last_frame_time = current_time;
    }
}

/**
 * @brief  Initialize CAN test generator
 * @param  None
 * @retval None
 */
void CANTestGenerator_Init(void)
{
    /* Initialize CAN driver */
    CAN_Init(500000); /* 500 kbit/s */
    
    last_frame_time = HAL_GetTick();
}
