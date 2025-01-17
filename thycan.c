#include "thycan.h"

// /* Global CAN State */
// CAN_State thycan_state = {
//     .front = 0,
//     .rear = 0,
//     .count = 0,
//     .sent = false,
//     .timeout = 1000 // Default timeout value
// };

/* Internal Functions */
static bool send_bits(uint32_t bit_end, uint32_t sample_point, CAN_Frame *frame);

/* Initialize the CAN peripheral */
void thycan_init(void) {
    // Configure GPIO pins for CAN TX and RX
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Configure CAN_TX_PIN
    GPIO_InitStruct.Pin = CAN_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(CAN_GPIO_PORT, &GPIO_InitStruct);

    // Configure CAN_RX_PIN
    GPIO_InitStruct.Pin = CAN_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(CAN_GPIO_PORT, &GPIO_InitStruct);
}

/* Sends a single CAN frame */
bool thycan_set_frame(CAN_State *state, CAN_Frame *frame) {
    // CAN_State *state = &thycan_state;

    if (state->count == CAN_QUEUE_SIZE) {
        // Queue is full, remove the front frame
        state->front = (state->front + 1) % CAN_QUEUE_SIZE;
        state->count--;
    }

    // Add frame to the queue
    state->queue[state->rear] = *frame;
    state->rear = (state->rear + 1) % CAN_QUEUE_SIZE;
    state->count++;

    return true;
}

/* Process the CAN frame queue */
void thycan_process_queue(CAN_State *state) {
    // CAN_State *state = &thycan_state;

    if (state->count == 0) {
        // No frames to process
        return;
    }

    // Get the front frame in the queue
    CAN_Frame *frame = &state->queue[state->front];

    uint32_t now = GET_CLOCK();
    uint32_t sample_point = now + SAMPLE_POINT_OFFSET;
    uint32_t bit_end = ADVANCE(sample_point, SAMPLE_TO_BIT_END);

    if (send_bits(bit_end, sample_point, frame)) {
        // Frame failed to send, retry or discard
        state->front = (state->front + 1) % CAN_QUEUE_SIZE;
        state->count--;
    } else {
        // Frame sent successfully
        state->front = (state->front + 1) % CAN_QUEUE_SIZE;
        state->count--;
    }
}

/* Internal function to send bits (bitstream transmission logic) */
static bool send_bits(uint32_t bit_end, uint32_t sample_point, CAN_Frame *frame) {
    uint8_t tx_index = 0;
    uint8_t tx = frame->tx_bitstream[tx_index++];
    uint8_t cur_tx = tx;

    while (1) {
        uint32_t now = GET_CLOCK();

        if (REACHED(now, bit_end)) {
            SET_CAN_TX(tx);
            bit_end = ADVANCE(bit_end, BIT_TIME);

            cur_tx = tx;
            tx = frame->tx_bitstream[tx_index++];

            if (tx_index >= frame->tx_bits) {
                SET_CAN_TX_REC();
                return false; // Frame successfully sent
            }
        }

        if (REACHED(now, sample_point)) {
            uint32_t rx = GET_CAN_RX();

            if (rx != cur_tx) {
                SET_CAN_TX_REC();
                return true; // Arbitration lost or error
            }

            sample_point = ADVANCE(sample_point, BIT_TIME);
        }
    }
}

