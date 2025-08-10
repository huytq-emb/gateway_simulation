/**
 ******************************************************************************
 * @file    can_drv.c
 * @brief   CAN driver implementation for STM32F407 Gateway ECU
 * @author  Automotive Firmware Engineer
 * @date    August 2025
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "can_drv.h"
#include "system_config.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define CAN_TIMEOUT_MS          100

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static CanFrame_t rx_buffer[CAN_RX_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;
static volatile uint16_t rx_count = 0;
static volatile CanError_t last_error = CAN_ERROR_NONE;

/* Private function prototypes -----------------------------------------------*/
static void CAN_ConfigureBitTiming(uint32_t baudrate);
static void CAN_ConfigureFilters(void);
static bool CAN_WaitForTxMailbox(void);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  Initialize CAN peripheral
 * @param  baudrate: CAN bus baudrate (e.g., 500000 for 500 kbit/s)
 * @retval true if successful, false otherwise
 */
bool CAN_Init(uint32_t baudrate)
{
    /* Enable CAN1 clock */
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    
    /* Request initialization mode */
    CAN1->MCR |= CAN_MCR_INRQ;
    
    /* Wait for initialization mode acknowledgment */
    uint32_t timeout = SystemCoreClock / 1000; /* 1 ms timeout */
    while (!(CAN1->MSR & CAN_MSR_INAK) && timeout--);
    if (timeout == 0) return false;
    
    /* Configure CAN options */
    CAN1->MCR = CAN_MCR_INRQ |         /* Initialization request */
                CAN_MCR_NART |          /* No automatic retransmission */
                CAN_MCR_AWUM |          /* Automatic wake-up mode */
                CAN_MCR_ABOM;           /* Automatic bus-off management */
    
    /* Configure bit timing */
    CAN_ConfigureBitTiming(baudrate);
    
    /* Configure receive filters */
    CAN_ConfigureFilters();
    
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
    if (timeout == 0) return false;
    
    /* Clear error flags */
    CAN_ClearError();
    
    return true;
}

/**
 * @brief  Send CAN frame
 * @param  id: CAN identifier
 * @param  data: Pointer to data bytes
 * @param  dlc: Data length code (0-8)
 * @retval true if successful, false otherwise
 */
bool CAN_Send(uint32_t id, const uint8_t* data, uint8_t dlc)
{
    if (dlc > 8 || data == NULL) return false;
    
    /* Find empty transmit mailbox */
    if (!CAN_WaitForTxMailbox()) return false;
    
    uint8_t mailbox = 0;
    if (CAN1->TSR & CAN_TSR_TME0) mailbox = 0;
    else if (CAN1->TSR & CAN_TSR_TME1) mailbox = 1;
    else if (CAN1->TSR & CAN_TSR_TME2) mailbox = 2;
    else return false;
    
    /* Configure identifier and DLC */
    CAN1->sTxMailBox[mailbox].TIR = (id << CAN_TI0R_STID_Pos);
    CAN1->sTxMailBox[mailbox].TDTR = dlc;
    
    /* Load data */
    uint32_t data_low = 0, data_high = 0;
    for (int i = 0; i < dlc && i < 4; i++) {
        data_low |= (data[i] << (i * 8));
    }
    for (int i = 4; i < dlc && i < 8; i++) {
        data_high |= (data[i] << ((i - 4) * 8));
    }
    CAN1->sTxMailBox[mailbox].TDLR = data_low;
    CAN1->sTxMailBox[mailbox].TDHR = data_high;
    
    /* Request transmission */
    CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ;
    
    return true;
}

/**
 * @brief  Receive CAN frame from buffer
 * @param  frame: Pointer to frame structure
 * @retval true if frame received, false if buffer empty
 */
bool CAN_Receive(CanFrame_t* frame)
{
    if (frame == NULL || rx_count == 0) return false;
    
    /* Disable interrupts for atomic operation */
    __disable_irq();
    
    /* Copy frame from buffer */
    *frame = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % CAN_RX_BUFFER_SIZE;
    rx_count--;
    
    __enable_irq();
    
    return true;
}

/**
 * @brief  Get number of frames in RX buffer
 * @retval Number of frames available
 */
uint16_t CAN_GetRxCount(void)
{
    return rx_count;
}

/**
 * @brief  Get last CAN error
 * @retval Last error code
 */
CanError_t CAN_GetLastError(void)
{
    return last_error;
}

/**
 * @brief  Clear CAN error flags
 */
void CAN_ClearError(void)
{
    last_error = CAN_ERROR_NONE;
    CAN1->ESR = 0; /* Clear error flags */
}

/**
 * @brief  CAN interrupt handler
 */
void CAN_IRQHandler(void)
{
    /* FIFO 0 message pending */
    if (CAN1->RF0R & CAN_RF0R_FMP0) {
        /* Check for buffer overflow */
        if (rx_count >= CAN_RX_BUFFER_SIZE) {
            last_error = CAN_ERROR_OVERRUN;
            /* Release FIFO message */
            CAN1->RF0R |= CAN_RF0R_RFOM0;
            return;
        }
        
        /* Read message from FIFO */
        CanFrame_t* frame = &rx_buffer[rx_head];
        
        /* Extract identifier and DLC */
        frame->id = (CAN1->sFIFOMailBox[0].RIR >> CAN_RI0R_STID_Pos) & 0x7FF;
        frame->dlc = CAN1->sFIFOMailBox[0].RDTR & CAN_RDT0R_DLC;
        
        /* Extract data */
        uint32_t data_low = CAN1->sFIFOMailBox[0].RDLR;
        uint32_t data_high = CAN1->sFIFOMailBox[0].RDHR;
        
        for (int i = 0; i < 4 && i < frame->dlc; i++) {
            frame->data[i] = (data_low >> (i * 8)) & 0xFF;
        }
        for (int i = 4; i < 8 && i < frame->dlc; i++) {
            frame->data[i] = (data_high >> ((i - 4) * 8)) & 0xFF;
        }
        
        frame->timestamp = HAL_GetTick();
        
        /* Update buffer pointers */
        rx_head = (rx_head + 1) % CAN_RX_BUFFER_SIZE;
        rx_count++;
        
        /* Release FIFO message */
        CAN1->RF0R |= CAN_RF0R_RFOM0;
    }
    
    /* FIFO 0 overrun */
    if (CAN1->RF0R & CAN_RF0R_FOVR0) {
        last_error = CAN_ERROR_OVERRUN;
        CAN1->RF0R |= CAN_RF0R_FOVR0; /* Clear flag */
    }
    
    /* Bus-off error */
    if (CAN1->MSR & CAN_MSR_ERRI) {
        if (CAN1->ESR & CAN_ESR_BOFF) {
            last_error = CAN_ERROR_BUS_OFF;
        } else if (CAN1->ESR & CAN_ESR_EPVF) {
            last_error = CAN_ERROR_ERROR_PASSIVE;
        } else if (CAN1->ESR & CAN_ESR_EWGF) {
            last_error = CAN_ERROR_WARNING;
        }
        CAN1->MSR |= CAN_MSR_ERRI; /* Clear error interrupt flag */
    }
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Configure CAN bit timing for specified baudrate
 * @param  baudrate: Target baudrate in bps
 */
static void CAN_ConfigureBitTiming(uint32_t baudrate)
{
    uint32_t prescaler, bs1, bs2, sjw;
    
    /* Calculate bit timing for 500 kbit/s with APB1 = 42 MHz */
    if (baudrate == 500000) {
        prescaler = 5;  /* Prescaler = 6 (5+1) */
        bs1 = 12;       /* BS1 = 13 tq (12+1) */
        bs2 = 1;        /* BS2 = 2 tq (1+1) */
        sjw = 0;        /* SJW = 1 tq (0+1) */
    } else {
        /* Default to 500 kbit/s if unsupported baudrate */
        prescaler = 5;
        bs1 = 12;
        bs2 = 1;
        sjw = 0;
    }
    
    CAN1->BTR = (sjw << CAN_BTR_SJW_Pos) |
                (bs1 << CAN_BTR_TS1_Pos) |
                (bs2 << CAN_BTR_TS2_Pos) |
                prescaler;
}

/**
 * @brief  Configure CAN receive filters
 */
static void CAN_ConfigureFilters(void)
{
    /* Enter filter initialization mode */
    CAN1->FMR |= CAN_FMR_FINIT;
    
    /* Configure filter 0 for Engine RPM (ID 0x100) */
    CAN1->FM1R &= ~CAN_FM1R_FBM0;      /* Identifier mask mode */
    CAN1->FS1R |= CAN_FS1R_FSC0;       /* 32-bit scale */
    CAN1->FFA1R &= ~CAN_FFA1R_FFA0;    /* FIFO 0 assignment */
    
    /* Set filter to accept IDs 0x100, 0x101, 0x102 */
    CAN1->sFilterRegister[0].FR1 = (CAN_FILTER_ID_ENGINE << 21);
    CAN1->sFilterRegister[0].FR2 = (0x7F8 << 21); /* Mask for 0x100-0x107 */
    
    /* Activate filter 0 */
    CAN1->FA1R |= CAN_FA1R_FACT0;
    
    /* Leave filter initialization mode */
    CAN1->FMR &= ~CAN_FMR_FINIT;
}

/**
 * @brief  Wait for available transmit mailbox
 * @retval true if mailbox available, false on timeout
 */
static bool CAN_WaitForTxMailbox(void)
{
    uint32_t timeout = CAN_TIMEOUT_MS * (SystemCoreClock / 1000000);
    
    while (!(CAN1->TSR & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2)) && timeout--);
    
    return (timeout > 0);
}
