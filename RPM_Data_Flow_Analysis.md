# Gateway ECU Data Flow Analysis: RPM Processing

## Overview
This document explains how RPM data flows through the Gateway ECU system in loopback mode, from generation to UART output display.

## Complete RPM Data Flow

### 1. RPM Data Generation
**Location:** `main_loopback.c` - `Gateway_SendTestFrames()` function
**Line:** ~275-285

```c
/* Send Engine RPM frame (ID 0x100) */
uint16_t rpm_raw = test_rpm * 4;  // Convert RPM to raw format
CAN1->sTxMailBox[mailbox].TIR = (0x100 << CAN_TI0R_STID_Pos);
CAN1->sTxMailBox[mailbox].TDTR = 8; /* DLC = 8 */
CAN1->sTxMailBox[mailbox].TDLR = (rpm_raw & 0xFF) | ((rpm_raw >> 8) << 8);
CAN1->sTxMailBox[mailbox].TDHR = 0;
CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ;  // Transmit request
```

**What happens:**
- `test_rpm` starts at 1000 and increments by 100 each cycle
- RPM is multiplied by 4 to create `rpm_raw` (automotive scaling)
- Data is packed into CAN frame with ID 0x100
- Frame is transmitted via CAN loopback

### 2. RPM Data Reception (Loopback)
**Location:** `stm32f4xx_it.c` - `CAN1_RX0_IRQHandler()`
**Line:** ~45-55

```c
void CAN1_RX0_IRQHandler(void)
{
    CanFrame_t frame;
    
    if (CAN_ReceiveFrame(&frame)) {
        /* Forward frame to PDU Router */
        Router_ProcessCanFrame(&frame);
    }
}
```

**What happens:**
- CAN interrupt fires when loopback frame is received
- Frame data is extracted into `CanFrame_t` structure
- Frame is forwarded to PDU Router for processing

### 3. RPM Data Routing
**Location:** `pdu_router.c` - `Router_ProcessCanFrame()`
**Line:** ~85-105

```c
bool Router_ProcessCanFrame(const CanFrame_t* frame)
{
    if (frame == NULL) return false;
    
    /* Update statistics */
    router_stats.frames_processed++;
    
    /* Check if this CAN ID should be routed to UART */
    if (IsCanIdRouted(frame->id)) {
        /* Convert CAN frame to UART message */
        UartMessage_t uart_msg;
        if (ConvertCanToUart(frame, &uart_msg)) {
            /* Send to UART */
            Router_SendUartMessage(&uart_msg);
            router_stats.frames_routed++;
        }
    }
    
    return true;
}
```

**What happens:**
- Router checks if CAN ID 0x100 (RPM) should be forwarded
- Converts CAN frame to UART message format
- Calls UART transmission function

### 4. RPM Data Conversion
**Location:** `pdu_router.c` - `ConvertCanToUart()`
**Line:** ~140-170

```c
static bool ConvertCanToUart(const CanFrame_t* can_frame, UartMessage_t* uart_msg)
{
    switch (can_frame->id) {
        case 0x100:  /* Engine RPM */
        {
            uint16_t rpm_raw = (can_frame->data[1] << 8) | can_frame->data[0];
            uint16_t rpm = rpm_raw / 4;  // Convert back to actual RPM
            
            sprintf((char*)uart_msg->data, "RPM,%u\r\n", rpm);
            uart_msg->length = strlen((char*)uart_msg->data);
            return true;
        }
        // ... other cases
    }
}
```

**What happens:**
- Extracts raw RPM value from CAN frame bytes
- Divides by 4 to get actual RPM value (reverse of generation scaling)
- Formats as "RPM,1000" string for UART transmission

### 5. RPM Data UART Transmission
**Location:** `pdu_router.c` - `Router_SendUartMessage()`
**Line:** ~200-210

```c
static void Router_SendUartMessage(const UartMessage_t* message)
{
    if (message != NULL && message->length > 0) {
        UART_Write((const char*)message->data);
    }
}
```

**What happens:**
- Calls UART driver to transmit the formatted RPM string
- This is where "RPM,1000" appears on your terminal

### 6. UART Output (Final Display)
**Location:** `uart_drv.c` - `UART_Write()`
**Line:** ~150-165

```c
void UART_Write(const char* data)
{
    if (data == NULL) return;
    
    while (*data) {
        /* Wait for transmit buffer to be empty */
        while (!(USART3->SR & USART_SR_TXE));
        
        /* Send character */
        USART3->DR = *data++;
    }
}
```

**What happens:**
- Sends each character of "RPM,1000" to USART3
- Characters appear on your terminal as the final output

## Key Code Lines for RPM Display

### Primary Line That Prints RPM:
**File:** `pdu_router.c`
**Function:** `ConvertCanToUart()`
**Line:** ~155
```c
sprintf((char*)uart_msg->data, "RPM,%u\r\n", rpm);
```

### RPM Value Source:
**File:** `main_loopback.c`
**Function:** `Gateway_SendTestFrames()`
**Line:** ~275
```c
uint16_t rpm_raw = test_rpm * 4;
```

### RPM Value Updates:
**File:** `main_loopback.c`
**Function:** `Gateway_SendTestFrames()`
**Line:** ~330-332
```c
test_rpm += 100;
if (test_rpm > 6000) test_rpm = 1000;
```

## Loopback Confirmation

### Yes, the RPM values are from loopback data:

1. **Generated locally:** Test RPM values (1000, 1100, 1200...) are created in `Gateway_SendTestFrames()`
2. **Transmitted via CAN:** Sent through CAN controller in loopback mode
3. **Received via CAN:** Loopback causes immediate reception of the same frame
4. **Processed by router:** Converted from CAN format back to readable text
5. **Displayed via UART:** Final output shows the round-trip data

## Data Flow Diagram

```
[test_rpm = 1000] 
       ↓
[RPM * 4 = 4000] (raw format)
       ↓
[CAN Frame ID:0x100, Data:[0x10,0x0F,...]]
       ↓
[CAN Loopback] 
       ↓
[CAN RX Interrupt]
       ↓
[PDU Router Processing]
       ↓
[raw_rpm / 4 = 1000] (convert back)
       ↓
[sprintf("RPM,%u", 1000)]
       ↓
[UART Output: "RPM,1000"]
       ↓
[Your Terminal Display]
```

## Timing Control

The RPM values update every **1000 main loop iterations** (~1 second):
- Counter reaches 1000 → Send new CAN frame
- RPM increments: 1000 → 1100 → 1200 → ... → 6000 → 1000 (cycles)
- Each complete cycle through the PDU router takes ~10-50ms

## Verification Points

To verify this is truly loopback data:
1. **Disconnect external CAN:** RPM values still appear (proves loopback)
2. **Check timing:** RPM updates every ~1 second (matches test frame timing)
3. **Check pattern:** Values follow 1000,1100,1200...6000,1000 sequence
4. **Check format:** Always "RPM,####" format from `sprintf()` conversion

The RPM data you see is definitely the result of the complete loopback process, proving your Gateway ECU's CAN→UART routing functionality works correctly!
