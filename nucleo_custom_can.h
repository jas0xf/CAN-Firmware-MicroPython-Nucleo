// from micropython/ports/stm32/boards/NUCLEO_F411RE/mpconfigboard.h 
// MICROPY_HW_CAN1_TX (pin_B9) // and pin_B9's number defined in /ports/stm32/mboot/mphalport.h
// MICROPY_HW_CAN1_RX (pin_B8) // and pin_B8's number defined in /ports/stm32/mboot/mphalport.h

#define CAN_TX_PIN               (9U)
#define CAN_RX_PIN               (8U)

#define BIT_TIME                 (249U) // 1Mbps
#define BAUD_500KBIT_PRESCALE    (1U)
#define SAMPLE_POINT_OFFSET      (150U)
#define SAMPLE_TO_BIT_END        (BIT_TIME - SAMPLE_POINT_OFFSET)


// CAN clock controll
#define REACHED(now, target)     ((now) >= (target))
#define ADVANCE(now, duration)   ((now) + (duration))

#define GET_CLOCK()                         (pwm_hw->slice[CANHACK_PWM].ctr)
#define RESET_CLOCK(t)                      (pwm_hw->slice[CANHACK_PWM].ctr = (t))
#define GET_GPIO(gpio)                      (!!((1ul << (gpio)) & sio_hw->gpio_in))
#define SET_GPIO(gpio, value)               {                                       \
                                                uint32_t mask = 1ul << (gpio);      \
                                                if (value)                          \
                                                    sio_hw->gpio_set = mask;        \
                                                else                                \
                                                    sio_hw->gpio_clr = mask;        \
                                            }


#define GET_CAN_RX()                        GET_GPIO(CAN_RX_PIN)
#define SET_CAN_TX(bit)                     SET_GPIO(CAN_TX_PIN, (bit))

#define CAN_MAX_BITS                        (128U)

typedef struct {
    uint8_t tx_bitstream[CAN_MAX_BITS];     ///< The bitstream of the CAN frame
    bool stuff_bit[CAN_MAX_BITS];           ///< Indicates if the corresponding bit is a stuff bit
    uint8_t tx_bits;                            ///< Number of  bits in the frame
    uint32_t tx_arbitration_bits;               ///< Number of bits in arbitartion (including stuff bits); the fields are ID A + RTR (standard) or ID A + SRR + IDE + ID B + RTR (extended)

    // Fields set when creating the CAN frame
    uint32_t crc_rg;                            ///< CRC value (15 bit value)
    uint32_t last_arbitration_bit;              ///< Bit index of last arbitration bit (always the RTR bit for both IDE = 0 and IDE = 1); may be a stuff bit
    uint32_t last_dlc_bit;                      ///< Bit index of last bit of DLC field; may be a stuff bit
    uint32_t last_data_bit;                     ///< Bit index of the last bit of the data field; may be a stuff bit
    uint32_t last_crc_bit;                      ///< Bit index of last bit of the CRC field; may be a stuff bit
    uint32_t last_eof_bit;                      ///< Bit index of the last bit of the EOF field; may be a stuff bit
    bool frame_set;                             ///< True when the frame has been set; may be a stuff bit

    // Fields used during creation of the CAN frame
    uint32_t dominant_bits;                     ///< Dominant bits in a row
    uint32_t recessive_bits;                    ///< Recessive bits in a row
    bool stuffing;                              ///< True if stuffing enabled
    bool crcing;                                ///< True if CRCing enabled
} can_frame_t;
