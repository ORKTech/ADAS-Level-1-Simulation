/**
 * @file flash_programmer.h
 * @brief Flash Programming Interface for PIC18F2520
 * @author ORKTech
 * @date 2026-04-25
 */

#ifndef FLASH_PROGRAMMER_H
#define FLASH_PROGRAMMER_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
   Flash Programmer Status Codes
   ============================================================================ */

typedef enum {
    FLASH_OK                = 0,
    FLASH_INVALID_ADDRESS   = 1,
    FLASH_ERASE_ERROR       = 2,
    FLASH_WRITE_ERROR       = 3,
    FLASH_VERIFY_ERROR      = 4,
    FLASH_TIMEOUT           = 5,
    FLASH_HW_ERROR          = 6
} flash_status_t;

/* ============================================================================
   Flash Programmer Operations
   ============================================================================ */

/**
 * @brief Initialize flash programmer
 * @return flash_status_t Status code
 */
flash_status_t flash_init(void);

/**
 * @brief Erase a page of flash memory
 * 
 * @param address Address within the page to erase
 * @return flash_status_t Status code
 * 
 * @note Address must be page-aligned (multiple of PAGE_SIZE)
 */
flash_status_t flash_erase_page(uint32_t address);

/**
 * @brief Write data to flash memory
 * 
 * @param address Starting address (must be word-aligned)
 * @param data Buffer containing data to write
 * @param length Number of bytes to write
 * @return flash_status_t Status code
 * 
 * @note Data is written in 2-byte words for PIC18F2520
 */
flash_status_t flash_write_row(uint32_t address, const uint8_t *data, uint16_t length);

/**
 * @brief Read data from flash memory
 * 
 * @param address Starting address
 * @param buffer Output buffer
 * @param length Number of bytes to read
 * @return flash_status_t Status code
 */
flash_status_t flash_read(uint32_t address, uint8_t *buffer, uint16_t length);

/**
 * @brief Verify flash memory against buffer
 * 
 * @param address Starting address
 * @param data Expected data buffer
 * @param length Number of bytes to verify
 * @return flash_status_t Status code (FLASH_OK if verified, FLASH_VERIFY_ERROR if mismatch)
 */
flash_status_t flash_verify(uint32_t address, const uint8_t *data, uint16_t length);

/**
 * @brief Calculate CRC16 of flash region
 * 
 * @param address Starting address
 * @param length Number of bytes
 * @param crc Output CRC value
 * @return flash_status_t Status code
 */
flash_status_t flash_crc16(uint32_t address, uint32_t length, uint16_t *crc);

/**
 * @brief Get error message for status code
 * 
 * @param status Status code
 * @return const char* Error message string
 */
const char* flash_get_error_string(flash_status_t status);

#endif /* FLASH_PROGRAMMER_H */
