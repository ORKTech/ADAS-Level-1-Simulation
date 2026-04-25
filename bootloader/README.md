"""markdown
# PIC18F2520 Bootloader with Intel HEX Support

A comprehensive bootloader implementation for PIC18F2520 microcontroller with full Intel HEX firmware update capability.

## Features

### Core Bootloader Features
- ✅ **Self-Programming Capability**: Uses internal Flash write/erase mechanisms
- ✅ **Memory Partitioning**: Protected bootloader section + application section
- ✅ **Reset Vector Redirection**: Jumps to bootloader on reset, decides to update or run app
- ✅ **Fail-Safe Mechanism**: Prevents bricking with error recovery
- ✅ **Timeout Mechanism**: Auto-jumps to app after 5 seconds with no input

### Firmware Update Features
- ✅ **Intel HEX File Parsing**: Complete HEX record parser with validation
- ✅ **Block-wise Flash Programming**: Efficient page-based write operations
- ✅ **Verification Mechanism**: Read-back verification and CRC16 validation
- ✅ **Write Protection**: Bootloader region is write-protected
- ✅ **Error Recovery**: Retry mechanisms and checksum validation

### Communication
- ✅ **UART Serial Communication**: 9600 baud, 8N1 default
- ✅ **ACK/NACK Handshaking**: Reliable command acknowledgment
- ✅ **Timeout Protection**: Prevents hanging on bad connections

## Memory Configuration

```
Flash Memory Layout (32KB total):
┌─────────────────────────────────────┐
│ 0x0000 - 0x07FF: Bootloader (2KB)  │ Protected
├─────────────────────────────────────┤
│ 0x0800 - 0xFFFF: Application (62KB)│ Updateable
└─────────────────────────────────────┘

Page Size: 64 bytes
Word Size: 16 bits (2 bytes)
```

## File Structure

```
bootloader/
├── bootloader_config.h      # Configuration constants
├── bootloader.h/c           # Main bootloader logic
├── hex_parser.h/c           # Intel HEX format parser
├── flash_programmer.h/c     # Flash memory interface
├── uart_comm.h/c            # Serial communication
├── main.c                   # Entry point
└── README.md               # This file
```

## Intel HEX Format Support

Supports all standard Intel HEX record types:

| Type | Name | Purpose |
|------|------|---------|
| 00 | Data | Firmware code/data bytes |
| 01 | End of File | Marks end of file |
| 04 | Extended Linear Address | Sets upper 16 bits of address |
| 05 | Start Linear Address | Specifies execution start (informational) |

### Example HEX Record
```
:10000000214601360121470136007EFE09D2190140
```

Breakdown:
- `:` - Start code
- `10` - Byte count (16 bytes)
- `0000` - Address (0x0000)
- `00` - Record type (Data)
- `214601...0140` - 16 bytes of data
- `40` - Checksum (two's complement)

## Building

### Prerequisites
- MPLAB X IDE 6.0+
- XC8 Compiler v2.36+
- PIC18F2520 Device Family Pack

### Compilation
```bash
# Using MPLAB X
xc8-cc -mcpu=18F2520 -O2 *.c -o bootloader.hex

# Or with MPLAB IDE
# File -> New Project -> Select PIC18F2520
# Add source files and build
```

### Hex File Generation
```bash
# Produces Intel HEX format suitable for bootloader
xc8-cc -mcpu=18F2520 --final=hex bootloader.c
```

## Usage

### Hardware Connection
```
PIC18F2520 Pins:
- RX (RC7) -> USB-UART RX
- TX (RC6) -> USB-UART TX
- GND      -> USB-UART GND
- 3.3V or 5V -> USB-UART VCC (if powered from USB)

Typical UART Module:
FT232 / CH340G / CP2102 module recommended
```

### Bootloader Operation

1. **Power On**: Microcontroller executes bootloader
2. **Initialization**: Bootloader initializes UART and flash
3. **Firmware Check**: Waits for Intel HEX records (5 second timeout)
4. **Programming**: Receives and programs HEX records
5. **Verification**: Verifies written data integrity
6. **Jump**: Either jumps to user app or continues waiting

### Sending Firmware Update

Using Python:
```python
import serial
import time

ser = serial.Serial('COM3', 9600, timeout=1)

# Read HEX file
with open('firmware.hex', 'r') as f:
    for line in f:
        if line.startswith(':'):
            ser.write(line.encode())
            ser.write(b'\r\n')
            ack = ser.read(1)
            if ack != b':':  # ':' is ACK
                print(f"NACK received for: {line.strip()}")
            time.sleep(0.01)
            
ser.close()
```

Using Linux minicom:
```bash
minicom -D /dev/ttyUSB0 -b 9600
# Send file: Ctrl-A, S (select file)
# Choose your .hex file
```

## API Reference

### Main Bootloader
```c
bootloader_status_t bootloader_init(void);
bootloader_status_t bootloader_run(void);
uint8_t bootloader_is_app_valid(void);
void bootloader_jump_to_app(void);
```

### HEX Parser
```c
void hex_parser_init(hex_parser_state_t *parser);
hex_status_t hex_parse_record(const char *line, hex_record_t *record);
uint8_t hex_verify_checksum(const hex_record_t *record);
```

### Flash Programmer
```c
flash_status_t flash_init(void);
flash_status_t flash_erase_page(uint32_t address);
flash_status_t flash_write_row(uint32_t address, const uint8_t *data, uint16_t length);
flash_status_t flash_read(uint32_t address, uint8_t *buffer, uint16_t length);
flash_status_t flash_verify(uint32_t address, const uint8_t *data, uint16_t length);
```

### UART Communication
```c
uart_status_t uart_init(const uart_config_t *config);
uart_status_t uart_send_byte(uint8_t byte);
uart_status_t uart_recv_byte(uint8_t *byte, uint16_t timeout_ms);
uart_status_t uart_recv_line(char *line, uint16_t max_length, uint16_t timeout_ms);
uart_status_t uart_send_ack(void);
uart_status_t uart_send_nack(void);
```

## Configuration

Edit `bootloader_config.h` to customize:

```c
/* Memory */
#define BOOTLOADER_SIZE         0x0800      /* 2KB */
#define APPLICATION_SIZE        0xF800      /* 62KB */

/* Communication */
#define UART_BAUD_RATE          9600
#define UART_RX_TIMEOUT_MS      5000

/* Features */
#define ENABLE_VERIFICATION     1
#define ENABLE_CHECKSUM         1
#define ENABLE_FAIL_SAFE        1
```

## Security Considerations

1. **Bootloader Protection**: Use PIC18F2520 write protection bits
2. **Code Verification**: Implement CRC/signature check on application
3. **Timeout**: Prevents accidental bootloader entry
4. **Fail-Safe**: Jumps to app even on minor errors
5. **Memory Isolation**: Bootloader space protected from application writes

## Troubleshooting

### No Response from Bootloader
- Check UART connection (TX/RX crossed?)
- Verify baud rate (9600 baud default)
- Ensure bootloader code is programmed
- Check reset circuit

### Verification Failures
- Corrupted HEX file
- Flash memory bit errors
- Program memory conflict
- Insufficient power supply

### Application Won't Run
- Application code not aligned to 0x0800
- Reset vector not configured correctly
- Stack overflow in bootloader code
- Watchdog timer issues

### Checksum Errors
- HEX file corruption during transfer
- Serial port noise
- Baud rate mismatch
- Timeout too short for transfer

## Performance

| Operation | Time | Size |
|-----------|------|------|
| Erase Page (64B) | ~2ms | - |
| Write Word (2B) | ~20µs | 2 bytes |
| Verify Page | ~1ms | - |
| UART at 9600 baud | 1.04ms/byte | - |

## Limitations

- One-way firmware transfer (bootloader → device only)
- No upload capability (cannot read back application)
- Fixed memory configuration (no EEPROM support)
- Limited to Intel HEX format
- No firmware compression
- No authentication/encryption

## Future Enhancements

- [ ] Xmodem protocol support
- [ ] EEPROM programming
- [ ] Firmware compression
- [ ] CRC32 verification
- [ ] Multiple application support (slot-based)
- [ ] Over-the-air (OTA) updates
- [ ] AES encryption support

## Testing

### Unit Tests
```c
// Include tests for:
- hex_parser: Valid/invalid records, checksums
- flash_programmer: Erase, write, verify operations
- uart_comm: Transmission, reception, timeouts
```

### Integration Tests
1. Program bootloader to device
2. Send valid HEX firmware
3. Verify programming
4. Jump to application
5. Verify application runs

## License

MIT License - Use freely in your projects

## References

- [Intel HEX Format Specification](https://en.wikipedia.org/wiki/Intel_HEX)
- [PIC18F2520 Datasheet](https://www.microchip.com/en-us/product/PIC18F2520)
- [PIC18F2520 Programmer's Reference Manual](https://www.microchip.com/design-centers/16-bit-pic-microcontrollers)
- [MPLAB X IDE User Guide](https://www.microchip.com/en-us/tools-resources/development-tools/mplab-x-ide)

---

**Author**: ORKTech  
**Version**: 1.0.0  
**Last Updated**: 2026-04-25
"""
