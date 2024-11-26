#include <stdio.h>
#include "nucleo_custom_can.h"
#include <py/runtime.h>  // in micropython source


typedef struct _can_custom_obj_t {
  mp_obj_base_t base;
  uint32_t bit_rate_kbps;
} can_custom_obj_t;

struct can {
    can_frame_t can_frame1;                 // CAN frame shared with API
    can_frame_t can_frame2;                 // CAN frame shared with API

    // Status
    bool sent;                                  // Indicates if frame sent or not

    struct {
        uint64_t bitstream_mask;
        uint64_t bitstream_match;
        uint32_t n_frame_match_bits;
        uint32_t n_frame_match_bits_cntdn;
        uint32_t attack_cntdn;
        uint32_t dominant_bit_cntdn;
    } attack_parameters;
};

static void add_bit(uint8_t bit, can_frame_t *frame);

STATIC bool can_send_frame(uint32_t retries);

STATIC mp_obj_t custom_can_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, MP_OBJ_FUN_ARGS_MAX, true);

    can_custom_obj_t *self = m_new_obj(can_rp2_obj_t);
    self->base.type = &custom_can_type;

    // Argument parsing
    enum { ARG_bit_rate };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_bit_rate, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 500} },
    };

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);

    mp_arg_val_t parsed_args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed_args);

    uint32_t bit_rate = parsed_args[ARG_bit_rate].u_int;
    // Validate bit rate
    if (bit_rate != 500 && bit_rate != 250 && bit_rate != 125) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Valid baud rates are 500, 250, 125 kbit/sec"));
    }

    // Store bit rate in object
    self->bit_rate_kbps = bit_rate;

    // Initialize GPIO and controller based on bit rate
    init_gpio();
    switch (bit_rate) {
        case 500U:
            init_ctr(BAUD_500KBIT_PRESCALE);
            break;
        case 250U:
            init_ctr(BAUD_250KBIT_PRESCALE);
            break;
        case 125U:
            init_ctr(BAUD_125KBIT_PRESCALE);
            break;
    }

    return MP_OBJ_FROM_PTR(self);
}


STATIC mp_obj_t custom_can_set_frame(mp_unit_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
  static const mp_arg_t allowed_args[] = {
    { MP_QSTR_can_id,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0x7ff} },
    { MP_QSTR_data,       MP_ARG_OBJ,                   {.u_obj = mp_const_none} },
    { MP_QSTR_remote,     MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    { MP_QSTR_dlc,        MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int  = 0} },
  };

  // Argument parsing
  mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
  mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

  uint32_t can_id = args[0].u_int;
  mp_obj_t data_obj = args[1].u_obj;
  bool rtr = args[2].u_bool;

  uint32_t len;
  uint32_t dlc;
  uint8_t data[8];

  if(data_obj == mp_const_none) {
      len = 0;
  }
  else {
      len = copy_mp_bytes(data_obj, data, 8U);
  }

  if(rtr && (len > 0)) {
      nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Remote frames cannot have a payload"));
  }
  // 8 byte frames max
  if (len > 8U) {
      nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Payload cannot be more than 8 bytes"));
  }

  dlc = len;

  can_frame_t *frame;
  uint8_t len = rtr ? 0 : (dlc >= 8U ? 8U : dlc);

    frame->tx_bits = 0;
    frame->crc_rg = 0;
    frame->stuffing = true;
    frame->crcing = true;
    frame->dominant_bits = 0;
    frame->recessive_bits = 0;

    for (uint32_t i = 0; i < CAN_MAX_BITS; i++) {
        frame->tx_bitstream[i] = 1U;
    }

    // ID field is:
    // {SOF, ID A, RTR, IDE = 0, r0} [Standard]

    // SOF
    add_bit(0, frame);

    // ID A
    id_a <<= 21U;
    for (uint32_t i = 0; i < 11U; i++) {
        if (id_a & 0x80000000U) {
            add_bit(1U, frame);
        }
        else {
            add_bit(0, frame);
        }
        id_a <<= 1U;
    }

    // RTR
    if (rtr) {
        add_bit(1U, frame);
    }
    else {
        add_bit(0, frame);
    }

    // The last bit of the arbitration field is the RTR bit if a basic frame
    frame->last_arbitration_bit = frame->tx_bits - 1U;

    add_bit(0, frame);

    // r0
    add_bit(0, frame);

    // DLC (length)
    dlc <<= 28U;
    for (uint32_t i = 0; i < 4U; i++) {
        if (dlc & 0x80000000U) {
            add_bit(1U, frame);
        } else {
            add_bit(0, frame);
        }
        dlc <<= 1U;
    }
    frame->last_dlc_bit = frame->tx_bits - 1U;

    // Data
    for (uint32_t i = 0; i < len; i ++) {
        uint8_t byte = data[i];
        for (uint32_t j = 0; j < 8; j++) {
            if (byte & 0x80U) {
                add_bit(1U, frame);
            }
            else {
                add_bit(0, frame);
            }
            byte <<= 1U;
        }
    }
    // If the length is 0 then the last data bit is equal to the last DLC bit
    frame->last_data_bit = frame->tx_bits - 1U;

    // CRC
    frame->crcing = false;
    uint32_t crc_rg = frame->crc_rg << 17U;
    for (uint32_t i = 0; i < 15U; i++) {
        if (crc_rg & 0x80000000U) {
            add_bit(1U, frame);
        } else {
            add_bit(0, frame);
        }
        crc_rg <<= 1U;
    }
    frame->last_crc_bit = frame->tx_bits - 1U;

  //
    // Bit stuffing is disabled at the end of the CRC field
    frame->stuffing = false;

    // CRC delimiter
    add_bit(1U, frame);
    // ACK slot
    add_bit(1U, frame);
    // ACK delimiter
    add_bit(1U, frame);
    // EOF
    add_bit(1U, frame);
    add_bit(1U, frame);
    add_bit(1U, frame);
    add_bit(1U, frame);
    add_bit(1U, frame);
    add_bit(1U, frame);
    add_bit(1U, frame);
    frame->last_eof_bit = frame->tx_bits - 1U;

    // IFS
    add_bit(1U, frame);
    add_bit(1U, frame);
    add_bit(1U, frame);

    // Set up the matching masks for this CAN frame
    frame->tx_arbitration_bits = frame->last_arbitration_bit + 1U;

    frame->frame_set = true;

  return mp_const_none;
}


STATIC mp_obj_t custom_can_send_frame(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
            { MP_QSTR_timeout,           MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 50000000U} },
            { MP_QSTR_retries,           MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 0} },
            { MP_QSTR_repeat,            MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 1U} }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint32_t timeout = args[0].u_int;
    uint32_t retries = args[1].u_int;
    uint32_t repeat = args[2].u_int;

    can_frame_t *frame = custom_can_get_frame();
    if (!frame->frame_set) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "CAN frame has not been set"));
    }

    // // Skip transmitting the EOF/IFS so (11 recessive bits) that can send frames back to back
    // frame->tx_bits -= 11U;
  
    for(;;) {
        // Disable interrupts around the library call because any interrupts will mess up the timing
        disable_irq();
        RESET_CLOCK(0);
        bool success = can_send_frame(retries, second);
        if (success) {
            repeat--;
        }
        enable_irq();
        if (!success || repeat == 0) {
            break;
        }
    }

    // // Put the 11 recessive bits back
    // frame->tx_bits += 11U;

    return mp_const_none;
}

STATIC bool send_bits(ctr_t bit_end, ctr_t sample_point, struct can *can_p, uint8_t tx_index, can_frame_t *can_frame);

STATIC bool can_send_frame(uint32_t retries){
  uint32_t prev_rx = 0;
    struct can *can_p = &can;
    can_frame_t *can_frame = &can_p->can_frame1;
    uint32_t bitstream = 0;
    uint8_t tx_index;

    // Look for 11 recessive bits or 10 recessive bits and a dominant
    uint8_t rx;
    RESET_CLOCK(0);
    ctr_t now;
    ctr_t sample_point = SAMPLE_POINT_OFFSET;
SOF:
    for (;;) {
        rx = GET_CAN_RX();
        now = GET_CLOCK();


        if (prev_rx && !rx) {
            RESET_CLOCK(0);
            sample_point = SAMPLE_POINT_OFFSET;
        }
        else if (REACHED(now, sample_point)) {
            ctr_t bit_end = ADVANCE(sample_point, SAMPLE_TO_BIT_END);
            sample_point = ADVANCE(now, BIT_TIME);

            bitstream = (bitstream << 1U) | rx;
            if ((bitstream & 0x7feU) == 0x7feU) {
                // 0x7fe = 11111111110
                // 11 bits, either 10 recessive and dominant = SOF, or 11 recessive
                // If the last bit was recessive then start index at 0, else start it at 1 to skip SOF
                tx_index = rx ^ 1U;
                if (send_bits(bit_end, sample_point, can_p, tx_index, can_frame)) {
                    if (retries--) {
                        bitstream = 0; // Make sure we wait until EOF+IFS to trigger next attempt
                        goto SOF;
                    }
                    return false;
                }
                return can_p->sent;
            }
        }
        prev_rx = rx;
    }
}

STATIC bool send_bits(ctr_t bit_end, ctr_t sample_point, struct can *can_p, uint8_t tx_index, can_frame_t *frame)
{
    ctr_t now;
    uint32_t rx;
    uint8_t tx = frame->tx_bitstream[tx_index++];
    uint8_t cur_tx = tx;

    for (;;) {
        now = GET_CLOCK();
        if (REACHED(now, bit_end)) {
            SET_CAN_TX(tx);
            bit_end = ADVANCE(bit_end, BIT_TIME);

            // The next bit is set up after the time because the critical I/O operation has taken place now
            cur_tx = tx;
            tx = frame->tx_bitstream[tx_index++];

            if ((tx_index >= frame->tx_bits)) {
                can_p->sent = true;
                return false;
            }
        }
        if (REACHED(now, sample_point)) {
            rx = GET_CAN_RX();
            if (rx != cur_tx) {
                    // If arbitration then lost, or an error, then give up and go back to SOF
                    SET_CAN_TX_REC();
                    return true;
            }
            sample_point = ADVANCE(sample_point, BIT_TIME);
        }
    }
}

static void add_raw_bit(uint8_t bit, bool stuff, can_frame_t *frame)
{
    // Record the status of the stuff bit for display purposes
    frame->stuff_bit[frame->tx_bits] = stuff;
    frame->tx_bitstream[frame->tx_bits++] = bit;
}

static void do_crc(uint8_t bitval, can_frame_t *frame)
{
    uint32_t bit_14 = (frame->crc_rg & (1U << 14U)) >> 14U;
    uint32_t crc_nxt = bitval ^ bit_14;
    frame->crc_rg <<= 1U;
    frame->crc_rg &= 0x7fffU;
    if (crc_nxt) {
        frame->crc_rg ^= 0x4599U;
    }
}

static void add_bit(uint8_t bit, can_frame_t *frame)
{
    if (frame->crcing) {
        do_crc(bit, frame);
    }
    add_raw_bit(bit, false, frame);
    if (bit) {
        frame->recessive_bits++;
        frame->dominant_bits = 0;
    } else {
        frame->dominant_bits++;
        frame->recessive_bits = 0;
    }
    if (frame->stuffing) {
        if (frame->dominant_bits >= 5U) {
            add_raw_bit(1U, true, frame);
            frame->dominant_bits = 0;
            frame->recessive_bits = 1U;
        }
        if (frame->recessive_bits >= 5U) {
            add_raw_bit(0, true, frame);
            frame->dominant_bits = 1U;
            frame->recessive_bits = 0;
        }
    }
}
