/**
 * @file flash_programmer.c
 * @brief Flash Programming Implementation for PIC18F2520
 * @author ORKTech
 * @date 2026-04-25
 * 
 * PIC18F2520 Flash Programming:
 * - Program memory: 32KB (0x0000 - 0x7FFF)
 * - Page size: 64 bytes
 * - Word size: 16 bits (2 bytes)
 * - TBLPTR (Table Pointer) register controls address
 * - TABLAT (Table Latch) holds data byte
 * - FSR0 used for addressing operations
 */

#include "flash_programmer.h"
#include "bootloader_config.h"

/* ============================================================================
   Private Defines for PIC18F2520 Flash Control
   ============================================================================ */

/* EECON1 Register bits */
#define RD_BIT          0   /* Read control bit */
#define WR_BIT          1   /* Write control bit */
#define WREN_BIT        2   /* Write enable bit */
#define WRERR_BIT       3   /* Write error bit */
#define FREE_BIT        4   /* Free bit (erase) */

/* Flash timing constants (in clock cycles) */
#define FLASH_WRITE_DELAY_US 2000   /* 2ms write delay */
#define FLASH_ERASE_DELAY_US 2000   /* 2ms erase delay */

/* ============================================================================
   Private Function Prototypes
   ============================================================================ */

static uint16_t flash_crc16_byte(uint16_t crc, uint8_t byte);
static void flash_wait_write_complete(void);
static flash_status_t flash_validate_address(uint32_t address);

/* ============================================================================
   Public Function Implementations
   ============================================================================ */

flash_status_t flash_init(void)
{
    /* Initialize flash programmer
     * Note: In a real implementation, this would configure:
     * - EECON1 register
     * - Interrupt priorities
     * - Protection bits (if applicable)
     */
    return FLASH_OK;
}

flash_status_t flash_erase_page(uint32_t address)
{
    flash_status_t status;
    
    /* Validate address */
    status = flash_validate_address(address);
    if (status != FLASH_OK) {
        return status;
    }
    
    /* Ensure address is page-aligned */
    if (address % PAGE_SIZE != 0) {
        return FLASH_INVALID_ADDRESS;
    }
    
    /* Note: In real implementation, would:
     * 1. Set TBLPTR to address
     * 2. Set FREE bit in EECON1 to enable page erase
     * 3. Set WREN bit
     * 4. Execute erase sequence (required unlock for PIC18)
     * 5. Wait for completion
     * 6. Clear WREN and FREE bits
     */
    
    flash_wait_write_complete();
    
    return FLASH_OK;
}

flash_status_t flash_write_row(uint32_t address, const uint8_t *data, uint16_t length)
{
    flash_status_t status;
    uint16_t i;
    
    if (data == NULL || length == 0) {
        return FLASH_INVALID_ADDRESS;
    }
    
    /* Validate address */
    status = flash_validate_address(address);
    if (status != FLASH_OK) {
        return status;
    }
    
    /* Write data in 2-byte words (PIC18F2520 uses 16-bit words) */
    for (i = 0; i < length; i += 2) {
        uint32_t word_address = address + i;
        uint16_t word_data;
        
        /* Construct 16-bit word from 2 bytes */
        word_data = (data[i + 1] << 8) | data[i];
        
        /* Note: In real implementation, would:
         * 1. Set TBLPTR to word_address
         * 2. Set TABLAT with lower byte
         * 3. Execute table latch operations
         * 4. Set WR bit to initiate write
         * 5. Execute unlock sequence
         * 6. Wait for completion
         */
    }
    
    flash_wait_write_complete();
    
    return FLASH_OK;
}

flash_status_t flash_read(uint32_t address, uint8_t *buffer, uint16_t length)
{
    flash_status_t status;
    uint16_t i;
    
    if (buffer == NULL || length == 0) {
        return FLASH_INVALID_ADDRESS;
    }
    
    /* Validate address */
    status = flash_validate_address(address);
    if (status != FLASH_OK) {
        return status;
    }
    
    /* Read data */
    for (i = 0; i < length; i++) {
        uint32_t read_address = address + i;
        
        /* Note: In real implementation, would:
         * 1. Set TBLPTR to read_address
         * 2. Execute TBLRD instruction to read byte
         * 3. Read TABLAT register
         * 4. Store in buffer
         */
        
        buffer[i] = 0x00;  /* Placeholder */
    }
    
    return FLASH_OK;
}

flash_status_t flash_verify(uint32_t address, const uint8_t *data, uint16_t length)
{
    flash_status_t status;
    uint8_t *read_buffer;
    uint16_t i;
    
    if (data == NULL || length == 0) {
        return FLASH_INVALID_ADDRESS;
    }
    
    /* Allocate temporary buffer for readback */
    uint8_t read_data[256];
    
    /* Read flash memory */
    status = flash_read(address, read_data, length);
    if (status != FLASH_OK) {
        return status;
    }
    
    /* Compare byte-by-byte */
    for (i = 0; i < length; i++) {
        if (read_data[i] != data[i]) {
            return FLASH_VERIFY_ERROR;
        }
    }
    
    return FLASH_OK;
}

flash_status_t flash_crc16(uint32_t address, uint32_t length, uint16_t *crc)
{
    flash_status_t status;
    uint8_t read_buffer[256];
    uint32_t bytes_read = 0;
    uint32_t current_address = address;
    uint16_t current_crc = 0xFFFF;
    uint16_t to_read;
    uint16_t i;
    
    if (crc == NULL || length == 0) {
        return FLASH_INVALID_ADDRESS;
    }
    
    /* Validate starting address */
    status = flash_validate_address(address);
    if (status != FLASH_OK) {
        return status;
    }
    
    /* Read and calculate CRC in chunks */
    while (bytes_read < length) {
        to_read = (length - bytes_read) > sizeof(read_buffer) ? 
                  sizeof(read_buffer) : (uint16_t)(length - bytes_read);
        
        status = flash_read(current_address, read_buffer, to_read);
        if (status != FLASH_OK) {
            return status;
        }
        
        /* Calculate CRC for this chunk */
        for (i = 0; i < to_read; i++) {
            current_crc = flash_crc16_byte(current_crc, read_buffer[i]);
        }
        
        current_address += to_read;
        bytes_read += to_read;
    }
    
    *crc = current_crc;
    return FLASH_OK;
}

const char* flash_get_error_string(flash_status_t status)
{
    const char *error_strings[] = {
        "No error",
        "Invalid address",
        "Erase error",
        "Write error",
        "Verification error",
        "Timeout",
        "Hardware error"
    };
    
    if (status > 6) {
        return "Unknown error";
    }
    
    return error_strings[status];
}

/* ============================================================================
   Private Function Implementations
   ============================================================================ */

static uint16_t flash_crc16_byte(uint16_t crc, uint8_t byte)
{
    uint8_t i;
    
    for (i = 0; i < 8; i++) {
        uint8_t bit = (byte >> i) & 1;
        uint8_t c15 = (crc >> 15) & 1;
        crc <<= 1;
        
        if (c15 ^ bit) {
            crc ^= 0x1021;  /* CRC-16-CCITT polynomial */
        }
    }
    
    return crc;
}

static void flash_wait_write_complete(void)
{
    /* In real implementation, would:
     * - Poll WR bit until clear
     * - Check for timeout
     * - Return timeout error if exceeded
     * For simulation, we just have a placeholder
     */
    unsigned int timeout = 10000;
    while (timeout--) {
        /* Check WR bit */
        /* if (!EECON1bits.WR) break; */
    }
}

static flash_status_t flash_validate_address(uint32_t address)
{
    /* Bootloader space is protected */
    if (address >= BOOTLOADER_START && address <= BOOTLOADER_END) {
        return FLASH_INVALID_ADDRESS;
    }
    
    /* Check if within application space */
    if (address < APPLICATION_START || address > APPLICATION_END) {
        return FLASH_INVALID_ADDRESS;
    }
    
    return FLASH_OK;
}
