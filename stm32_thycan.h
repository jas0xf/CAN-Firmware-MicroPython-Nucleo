#ifndef STM32_THYCAN_H
#define STM32_THYCAN_H

#include "py/obj.h"  // MicroPython's Python object header
#include "thycan.h"   // Include your custom CAN module

// Define the MicroPython CAN object structure
typedef struct _stm32_thycan_obj_t {
    mp_obj_base_t base; // Base class structure (required for all Python objects)
    CAN_State state;    // Internal CAN state
} stm32_thycan_obj_t;

// Define the Python class
extern const mp_obj_type_t stm32_thycan_type;

// Function declarations for the Python class methods
mp_obj_t stm32_thycan_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
mp_obj_t stm32_thycan_init(void);
mp_obj_t stm32_thycan_set_frame(mp_obj_t self_in, mp_obj_t frame_in);
mp_obj_t stm32_thycan_process_queue(mp_obj_t self_in);

#endif // STM32_THYCAN_H
