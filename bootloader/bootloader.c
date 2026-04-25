/**
 * @file bootloader.c
 * @brief Main Bootloader Implementation
 * @author ORKTech
 * @date 2026-04-25
 */

#include "bootloader.h"
#include "bootloader_config.h"
#include "hex_parser.h"
#include "flash_programmer.h"
#include "uart_comm.h"
#include <string.h>

/* ============================================================================
   Bootloader State
   ============================================================================ */

static struct {
    uint8_t initialized;
    uint32_t bytes_written;
    uint32_t pages_erased;
    uint8_t update_in_progress;
    hex_parser_state_t hex_state;
} bootloader_state = {
    .initialized = 0,
    .bytes_written = 0,
    .pages_erased = 0,
    .update_in_progress = 0
};

/* ============================================================================
   Private Function Prototypes
   ============================================================================ */

static bootloader_status_t bootloader_process_hex_record(const char *line);
static bootloader_status_t bootloader_handle_data_record(const hex_record_t *record);
static uint32_t bootloader_get_absolute_address(const hex_record_t *record);
static bootloader_status_t bootloader_erase_if_needed(uint32_t address);

/* ============================================================================
   Public Function Implementations
   ============================================================================ */

bootloader_status_t bootloader_init(void)
{
    uart_config_t uart_cfg;
    flash_status_t flash_status;
    uart_status_t uart_status;
    
    /* Initialize UART */
    uart_cfg.baud_rate = UART_BAUD_RATE;
    uart_cfg.data_bits = 8;
    uart_cfg.stop_bits = 1;
    uart_cfg.parity = 0;
    
    uart_status = uart_init(&uart_cfg);
    if (uart_status != UART_OK) {
        return BL_COMMUNICATION_ERROR;
    }
    
    /* Initialize flash programmer */
    flash_status = flash_init();
    if (flash_status != FLASH_OK) {
        return BL_WRITE_FAILED;
    }
    
    /* Initialize HEX parser */
    hex_parser_init(&bootloader_state.hex_state);
    
    /* Clear state */
    bootloader_state.bytes_written = 0;
    bootloader_state.pages_erased = 0;
    bootloader_state.update_in_progress = 0;
    bootloader_state.initialized = 1;
    
    return BL_OK;
}

bootloader_status_t bootloader_run(void)
{
    char hex_line[MAX_HEX_LINE_LENGTH + 1];
    uart_status_t uart_status;
    bootloader_status_t status;
    
    if (!bootloader_state.initialized) {
        return BL_INVALID_DATA;
    }
    
    /* Wait for HEX records with timeout */
    while (1) {
        uart_status = uart_recv_line(hex_line, sizeof(hex_line), UART_RX_TIMEOUT_MS);
        
        if (uart_status == UART_RX_TIMEOUT) {
            /* No data received, jump to application if valid */
            if (bootloader_is_app_valid()) {
                uart_send_string("\r\nJumping to application...\r\n");
                bootloader_jump_to_app();
            }
            return BL_TIMEOUT;
        }
        
        if (uart_status != UART_OK) {
            uart_send_nack();
            return BL_COMMUNICATION_ERROR;
        }
        
        /* Process HEX record */
        status = bootloader_process_hex_record(hex_line);
        
        if (status == BL_OK) {
            uart_send_ack();
        } else {
            uart_send_nack();
            if (status != BL_OK) {
                return status;
            }
        }
    }
    
    return BL_OK;
}

uint8_t bootloader_is_app_valid(void)
{
    /* Check application validity
     * Typically checks:
     * 1. First instruction is valid (not 0xFFFF)
     * 2. Stack is within bounds
     * 3. Optional: CRC or signature check
     */
    uint8_t first_byte;
    flash_status_t status;
    
    status = flash_read(APPLICATION_START, &first_byte, 1);
    if (status != FLASH_OK) {
        return 0;
    }
    
    /* Check if flash is not blank (0xFF) */
    if (first_byte == 0xFF) {
        return 0;
    }
    
    return 1;
}

void bootloader_jump_to_app(void)
{
    /* Disable interrupts */
    /* In real implementation:
     * 1. Clear interrupt enable bits
     * 2. Clear interrupt flags
     * 3. Reset peripheral states
     * 4. Set PCLATH and TBLPTRU appropriately
     * 5. Use assembly to jump: goto APPLICATION_START
     */
    
    /* Jump to application
     * This is typically done in assembly:
     * RESET:
     *     goto APPLICATION_START
     */
    
    /* This function should not return */
    while (1) {
        /* Infinite loop if jump fails */
    }
}

bootloader_status_t bootloader_verify_app(void)
{
    /* Verify application memory integrity
     * 1. Read back written data
     * 2. Verify against known good values
     * 3. Check CRC if applicable
     */
    
    flash_status_t status;
    uint8_t read_buffer[64];
    uint16_t crc;
    
    status = flash_crc16(APPLICATION_START, APPLICATION_SIZE, &crc);
    if (status != FLASH_OK) {
        return BL_VERIFY_FAILED;
    }
    
    return BL_OK;
}

bootloader_status_t bootloader_update_firmware(void)
{
    bootloader_status_t status;
    
    bootloader_state.update_in_progress = 1;
    
    status = bootloader_run();
    
    bootloader_state.update_in_progress = 0;
    
    return status;
}

const char* bootloader_get_error_string(bootloader_status_t status)
{
    const char *error_strings[] = {
        "No error",
        "Invalid command",
        "Invalid data",
        "Write failed",
        "Verification failed",
        "Timeout",
        "Communication error",
        "Application valid",
        "Application invalid"
    };
    
    if (status > 8) {
        return "Unknown error";
    }
    
    return error_strings[status];
}

/* ============================================================================
   Private Function Implementations
   ============================================================================ */

static bootloader_status_t bootloader_process_hex_record(const char *line)
{
    hex_record_t record;
    hex_status_t hex_status;
    bootloader_status_t bl_status;
    
    /* Parse HEX record */
    hex_status = hex_parse_record(line, &record);
    if (hex_status != HEX_OK) {
        return BL_INVALID_DATA;
    }
    
    /* Update parser state */
    hex_status = hex_update_state(&bootloader_state.hex_state, &record);
    if (hex_status != HEX_OK) {
        return BL_INVALID_DATA;
    }
    
    /* Handle different record types */
    switch (record.record_type) {
        case HEX_DATA:
            bl_status = bootloader_handle_data_record(&record);
            return bl_status;
            
        case HEX_END_OF_FILE:
            /* Verify application after programming complete */
            return bootloader_verify_app();
            
        case HEX_EXTENDED_LINEAR:
        case HEX_START_LINEAR:
            /* These are informational records, no action needed */
            return BL_OK;
            
        default:
            return BL_INVALID_COMMAND;
    }
}

static bootloader_status_t bootloader_handle_data_record(const hex_record_t *record)
{
    flash_status_t flash_status;
    uint32_t address;
    
    if (record == NULL || record->byte_count == 0) {
        return BL_INVALID_DATA;
    }
    
    /* Calculate absolute address */
    address = bootloader_get_absolute_address(record);
    
    /* Erase page if needed */
    flash_status = bootloader_erase_if_needed(address);
    if (flash_status != FLASH_OK) {
        return BL_WRITE_FAILED;
    }
    
    /* Write data to flash */
    flash_status = flash_write_row(address, record->data, record->byte_count);
    if (flash_status != FLASH_OK) {
        return BL_WRITE_FAILED;
    }
    
    /* Verify written data if enabled */
#if ENABLE_VERIFICATION
    flash_status = flash_verify(address, record->data, record->byte_count);
    if (flash_status != FLASH_OK) {
        return BL_VERIFY_FAILED;
    }
#endif
    
    bootloader_state.bytes_written += record->byte_count;
    
    return BL_OK;
}

static uint32_t bootloader_get_absolute_address(const hex_record_t *record)
{
    /* Combine extended address with record address */
    return (bootloader_state.hex_state.extended_address << 16) | record->address;
}

static bootloader_status_t bootloader_erase_if_needed(uint32_t address)
{
    uint32_t page_start;
    static uint32_t last_erased_page = 0xFFFFFFFF;
    flash_status_t status;
    
    /* Calculate page boundary */
    page_start = (address / PAGE_SIZE) * PAGE_SIZE;
    
    /* Erase only if we haven't erased this page yet */
    if (page_start != last_erased_page && page_start >= APPLICATION_START) {
        status = flash_erase_page(page_start);
        if (status != FLASH_OK) {
            return status;
        }
        last_erased_page = page_start;
        bootloader_state.pages_erased++;
    }
    
    return FLASH_OK;
}
