/**
 ******************************************************************************
 * @file    pdu_router.c
 * @brief   PDU Router implementation for STM32F407 Gateway ECU
 * @author  Automotive Firmware Engineer
 * @date    August 2025
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "pdu_router.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define MAX_OUTPUT_LENGTH       64
#define SIGNAL_TABLE_SIZE       3

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/**
 * @brief Signal mapping table
 * 
 * This table defines how CAN signals are extracted and formatted for UART output.
 * Each entry specifies:
 * - CAN ID to monitor
 * - Byte position and length in CAN frame
 * - Scaling and offset for engineering units conversion
 * - Format string for UART output
 */
static const SignalConfig_t signal_table[SIGNAL_TABLE_SIZE] = {
    /* Engine RPM: ID 0x100, bytes 0-1, scale /4, format: RPM,xxxx */
    {
        .can_id = 0x100,
        .start_byte = 0,
        .length = 2,
        .scale = 0.25f,         /* RPM = raw_value / 4 */
        .offset = 0.0f,
        .format_string = "RPM,%d\r\n",
        .signal_name = "Engine_RPM"
    },
    
    /* Engine Temperature: ID 0x101, byte 2, scale 1, offset -40°C */
    {
        .can_id = 0x101,
        .start_byte = 2,
        .length = 1,
        .scale = 1.0f,
        .offset = -40.0f,       /* Temp = raw_value - 40 */
        .format_string = "TEMP,%d\r\n",
        .signal_name = "Engine_Temp"
    },
    
    /* Vehicle Speed: ID 0x102, bytes 4-5, scale /10, format: SPEED,xxx */
    {
        .can_id = 0x102,
        .start_byte = 4,
        .length = 2,
        .scale = 0.1f,          /* Speed = raw_value / 10 */
        .offset = 0.0f,
        .format_string = "SPEED,%d\r\n",
        .signal_name = "Vehicle_Speed"
    }
};

static RouterStats_t router_stats = {0};

/* Private function prototypes -----------------------------------------------*/
static const SignalConfig_t* FindSignalConfig(uint32_t can_id);
static uint32_t ExtractSignalValue(const uint8_t* data, const SignalConfig_t* config);
static void FormatAndSendSignal(const SignalConfig_t* config, uint32_t raw_value);
static void SendErrorMessage(const char* error_type, const char* details);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  Initialize PDU Router
 * @param  None
 * @retval None
 */
void Router_Init(void)
{
    /* Clear statistics */
    Router_ClearStatistics();
    
    /* Send startup message */
    UART_Write("Gateway ECU Started\r\n");
    UART_Write("Monitoring CAN IDs: 0x100, 0x101, 0x102\r\n");
}

/**
 * @brief  Process received CAN frame
 * @param  frame: Pointer to CAN frame
 * @retval None
 */
void Router_ProcessCanFrame(const CanFrame_t* frame)
{
    if (frame == NULL) return;
    
    router_stats.frames_processed++;
    
    /* Find signal configuration for this CAN ID */
    const SignalConfig_t* config = FindSignalConfig(frame->id);
    if (config == NULL) {
        router_stats.frames_dropped++;
        return;
    }
    
    /* Validate DLC */
    if (frame->dlc < (config->start_byte + config->length)) {
        router_stats.frames_dropped++;
        char error_msg[64];
        sprintf(error_msg, "CAN_ERR,INVALID_DLC,ID:0x%03X\r\n", (unsigned int)frame->id);
        UART_Write(error_msg);
        return;
    }
    
    /* Extract signal value */
    uint32_t raw_value = ExtractSignalValue(frame->data, config);
    
    /* Format and send via UART */
    FormatAndSendSignal(config, raw_value);
    
    router_stats.frames_routed++;
}

/**
 * @brief  Poll router for periodic tasks
 * @param  None
 * @retval None
 */
void Router_Poll(void)
{
    /* Check for CAN errors */
    CanError_t can_error = CAN_GetLastError();
    if (can_error != CAN_ERROR_NONE) {
        router_stats.can_errors++;
        
        switch (can_error) {
            case CAN_ERROR_BUS_OFF:
                SendErrorMessage("CAN_ERR", "BUS_OFF");
                break;
            case CAN_ERROR_ERROR_PASSIVE:
                SendErrorMessage("CAN_ERR", "ERROR_PASSIVE");
                break;
            case CAN_ERROR_WARNING:
                SendErrorMessage("CAN_ERR", "WARNING");
                break;
            case CAN_ERROR_OVERRUN:
                SendErrorMessage("CAN_ERR", "OVERRUN");
                break;
            default:
                SendErrorMessage("CAN_ERR", "UNKNOWN");
                break;
        }
        
        CAN_ClearError();
    }
    
    /* Check for UART errors */
    UartError_t uart_error = UART_GetLastError();
    if (uart_error != UART_ERROR_NONE) {
        router_stats.uart_errors++;
        
        switch (uart_error) {
            case UART_ERROR_OVERRUN:
                SendErrorMessage("UART_ERR", "OVERRUN");
                break;
            case UART_ERROR_FRAMING:
                SendErrorMessage("UART_ERR", "FRAMING");
                break;
            case UART_ERROR_PARITY:
                SendErrorMessage("UART_ERR", "PARITY");
                break;
            case UART_ERROR_BUFFER_FULL:
                SendErrorMessage("UART_ERR", "BUFFER_FULL");
                break;
            default:
                SendErrorMessage("UART_ERR", "UNKNOWN");
                break;
        }
        
        UART_ClearError();
    }
}

/**
 * @brief  Get router statistics
 * @param  stats: Pointer to statistics structure
 * @retval None
 */
void Router_GetStatistics(RouterStats_t* stats)
{
    if (stats != NULL) {
        *stats = router_stats;
    }
}

/**
 * @brief  Clear router statistics
 * @param  None
 * @retval None
 */
void Router_ClearStatistics(void)
{
    memset(&router_stats, 0, sizeof(RouterStats_t));
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Find signal configuration for CAN ID
 * @param  can_id: CAN identifier
 * @retval Pointer to signal configuration, NULL if not found
 */
static const SignalConfig_t* FindSignalConfig(uint32_t can_id)
{
    for (int i = 0; i < SIGNAL_TABLE_SIZE; i++) {
        if (signal_table[i].can_id == can_id) {
            return &signal_table[i];
        }
    }
    return NULL;
}

/**
 * @brief  Extract signal value from CAN data
 * @param  data: Pointer to CAN data bytes
 * @param  config: Signal configuration
 * @retval Extracted raw signal value
 */
static uint32_t ExtractSignalValue(const uint8_t* data, const SignalConfig_t* config)
{
    uint32_t value = 0;
    
    /* Extract bytes based on signal length */
    switch (config->length) {
        case 1:
            value = data[config->start_byte];
            break;
            
        case 2:
            /* Little-endian extraction */
            value = (data[config->start_byte + 1] << 8) | data[config->start_byte];
            break;
            
        case 4:
            /* Little-endian extraction */
            value = (data[config->start_byte + 3] << 24) |
                   (data[config->start_byte + 2] << 16) |
                   (data[config->start_byte + 1] << 8) |
                   data[config->start_byte];
            break;
            
        default:
            value = 0;
            break;
    }
    
    return value;
}

/**
 * @brief  Format signal value and send via UART
 * @param  config: Signal configuration
 * @param  raw_value: Raw signal value
 * @retval None
 */
static void FormatAndSendSignal(const SignalConfig_t* config, uint32_t raw_value)
{
    char output_buffer[MAX_OUTPUT_LENGTH];
    
    /* Apply scaling and offset */
    float eng_value = (raw_value * config->scale) + config->offset;
    int32_t rounded_value = (int32_t)(eng_value + 0.5f); /* Round to nearest integer */
    
    /* Format according to configuration */
    int length = sprintf(output_buffer, config->format_string, rounded_value);
    
    /* Ensure string is properly terminated and within bounds */
    if (length > 0 && length < MAX_OUTPUT_LENGTH) {
        UART_Write(output_buffer);
    }
}

/**
 * @brief  Send error message via UART
 * @param  error_type: Error type string
 * @param  details: Error details string
 * @retval None
 */
static void SendErrorMessage(const char* error_type, const char* details)
{
    char error_buffer[MAX_OUTPUT_LENGTH];
    int length = sprintf(error_buffer, "%s,%s\r\n", error_type, details);
    
    if (length > 0 && length < MAX_OUTPUT_LENGTH) {
        UART_Write(error_buffer);
    }
}
