#ifndef THYCAN_H
#define THYCAN_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal.h"

/* Timing Constants */
#define CAN_BITRATE          500000        // CAN bus speed in bps
#define BIT_TIME             (1000000 / CAN_BITRATE) // Time per bit in microseconds
#define SAMPLE_POINT_OFFSET  (BIT_TIME / 2) // Midpoint of the bit for sampling
#define SAMPLE_TO_BIT_END    (BIT_TIME - SAMPLE_POINT_OFFSET) // Time from sample point to bit end

// Define constants for the queue size
#define CAN_QUEUE_SIZE 16       // Size of the queue for CAN frames

// CAN Frame structure
typedef struct {
    uint32_t id;               // Identifier (Standard or Extended)
    uint8_t dlc;               // Data length code
    uint8_t data[8];           // Data payload (up to 8 bytes)
    bool extended;             // Whether the frame uses extended ID
    bool rtr;                  // Remote Transmission Request
    uint8_t tx_bitstream[128]; // Transmitted bitstream (calculated)
    uint8_t tx_bits;           // Number of bits in the frame
} CAN_Frame;

// CAN State structure with a queue
typedef struct {
    CAN_Frame queue[CAN_QUEUE_SIZE]; // Queue of CAN frames
    uint8_t front;                   // Front index of the queue
    uint8_t rear;                    // Rear index of the queue
    uint8_t count;                   // Current number of frames in the queue
    bool sent;                       // Frame sent flag
    uint32_t timeout;                // Timeout counter
} CAN_State;

// CAN TX/RX GPIO Pin Definitions
#define CAN_TX_PIN   GPIO_PIN_9  // CAN TX pin (PB9)
#define CAN_RX_PIN   GPIO_PIN_8  // CAN RX pin (PB8)
#define CAN_GPIO_PORT GPIOB 

// CAN Control Macros (Pin Manipulation)
#define SET_CAN_TX(bit)    HAL_GPIO_WritePin(CAN_GPIO_PORT, CAN_TX_PIN, (bit) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SET_CAN_TX_REC()   SET_CAN_TX(0)  // Set to dominant bit (logic '0')
#define SET_CAN_TX_IDLE()  SET_CAN_TX(1)  // Set to recessive bit (logic '1')
#define GET_CAN_RX()       HAL_GPIO_ReadPin(CAN_GPIO_PORT, CAN_RX_PIN)  // Read CAN RX pin state

/* Clock Management Macros */
#define GET_CLOCK()             HAL_GetTick()  // Get the current clock in milliseconds
#define RESET_CLOCK(x)          ((x) = HAL_GetTick())
#define ADVANCE(clock, offset)  ((clock) + (offset)) // Advance the clock by an offset
#define REACHED(now, target)    ((int32_t)((now) - (target)) >= 0) // Check if the target time is reached

// Function declarations
void thycan_init(void);
bool thycan_set_frame(CAN_State *state, CAN_Frame *frame);
void thycan_process_queue(CAN_State *state);

#endif // THYCAN_H
