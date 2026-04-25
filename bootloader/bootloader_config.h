/**
 * @file bootloader_config.h
 * @brief Bootloader Configuration for PIC18F2520
 * @author ORKTech
 * @date 2026-04-25
 */

#ifndef BOOTLOADER_CONFIG_H
#define BOOTLOADER_CONFIG_H

/* ============================================================================
   PIC18F2520 Memory Configuration
   ============================================================================ */

#define BOOTLOADER_START        0x0000      /* Bootloader starts at 0x0000 */
#define BOOTLOADER_SIZE         0x0800      /* 2KB bootloader space (0x0000-0x07FF) */
#define BOOTLOADER_END          (BOOTLOADER_START + BOOTLOADER_SIZE - 1)

#define APPLICATION_START       0x0800      /* Application starts at 0x0800 */
#define APPLICATION_SIZE        0xF800      /* Remaining flash for application */
#define APPLICATION_END         (APPLICATION_START + APPLICATION_SIZE - 1)

#define TOTAL_FLASH             0x10000     /* 64KB Flash for PIC18F2520 */

/* Program Memory Page Size (smallest erasable unit) */
#define PAGE_SIZE               64          /* 64 bytes per page */

/* ============================================================================
   UART Configuration
   ============================================================================ */

#define UART_BAUD_RATE          9600        /* Serial communication baud rate */
#define UART_RX_TIMEOUT_MS      5000        /* 5 seconds timeout for bootloader mode */

/* ============================================================================
   Bootloader Control
   ============================================================================ */

#define BOOTLOADER_MAGIC_BYTE   0xAB        /* Magic byte to trigger bootloader */
#define MAX_HEX_LINE_LENGTH     140         /* Maximum HEX record length */
#define CHECKSUM_ERROR_RETRY    3           /* Retry attempts for checksum errors */

/* ============================================================================
   Feature Flags
   ============================================================================ */

#define ENABLE_UART_COMMS       1           /* Enable UART communication */
#define ENABLE_VERIFICATION     1           /* Enable post-write verification */
#define ENABLE_CHECKSUM         1           /* Enable checksum validation */
#define ENABLE_TIMEOUT          1           /* Enable auto-jump timeout */
#define ENABLE_FAIL_SAFE        1           /* Enable fail-safe mechanism */

#endif /* BOOTLOADER_CONFIG_H */
