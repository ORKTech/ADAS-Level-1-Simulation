/**
 * @file main.c
 * @brief Bootloader Entry Point
 * @author ORKTech
 * @date 2026-04-25
 * 
 * PIC18F2520 Bootloader
 * 
 * This bootloader:
 * 1. Initializes communication and flash
 * 2. Waits for Intel HEX firmware records
 * 3. Programs received data into flash memory
 * 4. Verifies written data
 * 5. Jumps to user application or times out
 * 
 * Memory Map:
 * 0x0000 - 0x07FF: Bootloader (2KB)
 * 0x0800 - 0xFFFF: User Application (62KB)
 */

#include "bootloader.h"
#include "uart_comm.h"

/* ============================================================================
   Reset Vector Redirection
   ============================================================================
   
   On PIC18F2520:
   - Reset vector is at 0x0000
   - This must jump to bootloader code
   - Bootloader decides whether to update firmware or jump to app
   
   In assembly (typically in crt0.s or startup code):
   
   RESET:
       goto BOOTLOADER_START    ; Jump to bootloader at 0x0000
   
   BOOTLOADER_ENTRY:
       ; ... bootloader code ...
   
   USER_APPLICATION:
       goto APPLICATION_START   ; Jump to application at 0x0800
*/

/* ============================================================================
   Main Function
   ============================================================================ */

void main(void)
{
    bootloader_status_t status;
    
    /* Initialize bootloader */
    status = bootloader_init();
    
    if (status != BL_OK) {
        /* Initialization failed - try to jump to app anyway */
        if (bootloader_is_app_valid()) {
            bootloader_jump_to_app();
        }
        /* Halt if app invalid */
        while (1);
    }
    
    /* Send startup message (optional, for debugging) */
    uart_send_string("\r\n=== PIC18F2520 Bootloader ===\r\n");
    uart_send_string("Version 1.0.0\r\n");
    uart_send_string("Waiting for firmware update...\r\n");
    uart_send_string("(Send Intel HEX records or wait for timeout)\r\n\r\n");
    
    /* Run bootloader main loop */
    status = bootloader_run();
    
    /* If we reach here, either:
     * 1. Timeout occurred and app is valid (already jumped)
     * 2. Some error occurred
     */
    
    switch (status) {
        case BL_TIMEOUT:
            /* This shouldn't be reached if app is valid */
            uart_send_string("Bootloader timeout - application invalid\r\n");
            break;
            
        case BL_COMMUNICATION_ERROR:
            uart_send_string("Communication error\r\n");
            break;
            
        case BL_WRITE_FAILED:
            uart_send_string("Flash write failed\r\n");
            break;
            
        case BL_VERIFY_FAILED:
            uart_send_string("Verification failed\r\n");
            break;
            
        default:
            uart_send_string("Bootloader error\r\n");
            break;
    }
    
    /* Try to jump to application even on error (fail-safe) */
    if (bootloader_is_app_valid()) {
        uart_send_string("Jumping to application...\r\n");
        bootloader_jump_to_app();
    }
    
    /* Halt if unable to proceed */
    uart_send_string("FATAL: Cannot continue\r\n");
    while (1);
}

/* ============================================================================
   Interrupt Handlers (Minimal Implementation)
   ============================================================================ */

/* High-priority interrupt handler */
void interrupt_high(void)
{
    /* Typically used for time-critical tasks
     * In bootloader, usually kept minimal
     */
}

/* Low-priority interrupt handler */
void interrupt_low(void)
{
    /* Low-priority interrupts
     * In bootloader, usually empty or minimal
     */
}
