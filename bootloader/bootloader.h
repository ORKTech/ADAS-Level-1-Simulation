/**
 * @file bootloader.h
 * @brief Main Bootloader Control Module
 * @author ORKTech
 * @date 2026-04-25
 */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>

/* ============================================================================
   Bootloader Status Codes
   ============================================================================ */

typedef enum {
    BL_OK                  = 0,
    BL_INVALID_COMMAND     = 1,
    BL_INVALID_DATA        = 2,
    BL_WRITE_FAILED        = 3,
    BL_VERIFY_FAILED       = 4,
    BL_TIMEOUT             = 5,
    BL_COMMUNICATION_ERROR = 6,
    BL_APPLICATION_VALID   = 7,
    BL_APPLICATION_INVALID = 8
} bootloader_status_t;

/* ============================================================================
   Bootloader Version
   ============================================================================ */

#define BOOTLOADER_VERSION_MAJOR 1
#define BOOTLOADER_VERSION_MINOR 0
#define BOOTLOADER_VERSION_BUILD 0

/* ============================================================================
   Function Prototypes
   ============================================================================ */

/**
 * @brief Initialize bootloader
 * @return bootloader_status_t Status code
 */
bootloader_status_t bootloader_init(void);

/**
 * @brief Main bootloader command handler
 * 
 * Processes HEX records received via UART and programs them into flash memory
 * 
 * @return bootloader_status_t Status code
 */
bootloader_status_t bootloader_run(void);

/**
 * @brief Check if application is valid
 * 
 * @return uint8_t 1 if valid, 0 if not
 */
uint8_t bootloader_is_app_valid(void);

/**
 * @brief Jump to user application
 * 
 * Transfers execution to the application at APPLICATION_START address.
 * This function does not return.
 */
void bootloader_jump_to_app(void);

/**
 * @brief Verify entire application region
 * 
 * @return bootloader_status_t Status code
 */
bootloader_status_t bootloader_verify_app(void);

/**
 * @brief Process a single firmware update session
 * 
 * @return bootloader_status_t Status code
 */
bootloader_status_t bootloader_update_firmware(void);

/**
 * @brief Get error message for status code
 * 
 * @param status Status code
 * @return const char* Error message string
 */
const char* bootloader_get_error_string(bootloader_status_t status);

#endif /* BOOTLOADER_H */
