# Custom CAN Protocol Implementation for NUCLEO F446RE

## Project Overview

This project provides a low-level CAN (Controller Area Network) implementation for the NUCLEO F446RE microcontroller, designed as a MicroPython extension module. The implementation focuses on bit-level manipulation and custom frame construction for CAN 2.0 standard messaging.

## Features

### Hardware Configuration
- **Microcontroller**: NUCLEO F446RE
- **CAN Pins**: 
  - TX: GPIO Pin B9 (Pin 9)
  - RX: GPIO Pin B8 (Pin 8)

### CAN Protocol Specifications
- Supports CAN 2.0 standard frame format
- Configurable baud rates: 125 kbit/s, 250 kbit/s, 500 kbit/s
- Maximum payload: 8 bytes
- Support for standard (11-bit) CAN IDs
- Support for both data and remote frames

## Key Implementation Details

### Bit-Level Frame Construction
- Custom `add_bit()` function for precise frame building
- Handles bit stuffing mechanism
- CRC calculation integrated into frame construction

### Synchronization Mechanism
- Direct GPIO pin reading (pins 8 and 9)
- Custom timing control using PWM slice counter
- Sample point and bit timing configuration

### Frame Types
- **Data Frames**: Payload up to 8 bytes
- **Remote Frames**: No payload, used for requesting data

## Performance Characteristics
- Bit Rate: Configurable up to 1 Mbps
- Sampling Point: Configurable (default offset at 150 ticks)
- Precise bit-level control

## Python Module Functions

### `custom_can_set_frame()`
- Set CAN frame parameters
- Configure CAN ID
- Set payload data
- Choose frame type (data/remote)

### `custom_can_send_frame()`
- Send configured CAN frame
- Configurable timeout and retry mechanism

## Usage Example

```python
# Create CAN object with 500 kbit/s baud rate
can = CustomCAN(bit_rate=500)

# Prepare a CAN frame
can.set_frame(
    can_id=0x123,        # 11-bit CAN identifier
    data=b'\x01\x02\x03',# Payload (optional)
    remote=False,        # Data frame
    dlc=3                # Data length
)

# Send the frame
can.send_frame(
    timeout=50000000,    # Timeout in clock cycles
    retries=3,           # Number of retry attempts
    repeat=1             # Number of times to send
)
```



## Development Environment
- MicroPython
- STM32 HAL
- GCC ARM Embedded Toolchain
