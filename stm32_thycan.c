#include "py/obj.h"
#include "py/runtime.h"
#include "stm32_thycan.h"

// Helper function to initialize the CAN interface (wraps thycan_init)
mp_obj_t stm32_thycan_init(void) {
    // Initialize CAN using thycan_init
    thycan_init();
    
    return mp_const_none; // Return None after initialization
}

// Helper function to set a CAN frame (wraps thycan_set_frame)
mp_obj_t stm32_thycan_set_frame(mp_obj_t self_in, mp_obj_t frame_in) {
    stm32_thycan_obj_t *self = MP_OBJ_TO_PTR(self_in);
    CAN_Frame *frame = MP_OBJ_TO_PTR(frame_in); // Assuming 'frame_in' is a pointer to a CAN_Frame
    
    if (!thycan_set_frame(&self->state, frame)) {
        mp_obj_t msg = mp_obj_new_str("CAN frame set failed", strlen("CAN frame set failed"));
        mp_raise_msg(&mp_type_Exception, msg);
    }
    
    return mp_const_none; // Return None if the set was successful
}

// Helper function to process the CAN frame queue (wraps thycan_process_queue)
mp_obj_t stm32_thycan_process_queue(mp_obj_t self_in) {
    stm32_thycan_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    thycan_process_queue(&self->state);
    
    return mp_const_none; // Return None after processing the queue
}

// Constructor to create a new stm32_thycan object
mp_obj_t stm32_thycan_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    stm32_thycan_obj_t *self = mp_obj_malloc(stm32_thycan_obj_t, &stm32_thycan_type);
    self->base.type = &stm32_thycan_type;
    
    // Initialize the internal CAN state if necessary
    self->state.front = 0;
    self->state.rear = 0;
    self->state.count = 0;
    self->state.sent = false;
    self->state.timeout = 1000;  // Default timeout value
    
    return MP_OBJ_FROM_PTR(self);
}

// Define the methods of the stm32_thycan class
static MP_DEFINE_CONST_FUN_OBJ_0(stm32_thycan_init_obj, stm32_thycan_init);
static MP_DEFINE_CONST_FUN_OBJ_2(stm32_thycan_set_frame_obj, stm32_thycan_set_frame);
static MP_DEFINE_CONST_FUN_OBJ_1(stm32_thycan_process_queue_obj, stm32_thycan_process_queue);


static const mp_map_elem_t stm32_thycan_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_init), (mp_obj_t)&stm32_thycan_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_frame), (mp_obj_t)&stm32_thycan_set_frame_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_process_queue), (mp_obj_t)&stm32_thycan_process_queue_obj },
};

static MP_DEFINE_CONST_DICT(stm32_thycan_locals_dict, stm32_thycan_locals_dict_table);

;
