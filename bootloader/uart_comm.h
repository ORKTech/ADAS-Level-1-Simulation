/**
 * @file uart_comm.h
 * @brief UART Communication Interface for Bootloader
 * @author ORKTech
 * @date 2026-04-25
 */

#ifndef UART_COMM_H
#define UART_COMM_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
   UART Status Codes
   ============================================================================ */

typedef enum {
    UART_OK             = 0,
    UART_TX_ERROR       = 1,
    UART_RX_TIMEOUT     = 2,
    UART_RX_OVERFLOW    = 3,
    UART_FRAME_ERROR    = 4,
    UART_PARITY_ERROR   = 5,
    UART_NOT_READY      = 6
} uart_status_t;

/* ============================================================================
   UART Control Commands
   ============================================================================ */

typedef enum {
    CMD_ERASE_PAGE      = 0x01,  /* Erase a flash page */
    CMD_WRITE_DATA      = 0x02,  /* Write hex data record */
    CMD_VERIFY_PAGE     = 0x03,  /* Verify written data */
    CMD_READ_MEMORY     = 0x04,  /* Read memory region */
    CMD_GET_VERSION     = 0x05,  /* Get bootloader version */
    CMD_JUMP_APP        = 0x06,  /* Jump to application */
    CMD_NACK            = 0x0F,  /* Negative acknowledgment */
    CMD_ACK             = 0x3A   /* Positive acknowledgment (':') */
} uart_command_t;

/* ============================================================================
   UART Configuration
   ============================================================================ */

typedef struct {
    uint16_t baud_rate;           /* Baud rate in bits per second */
    uint8_t data_bits;            /* Data bits (usually 8) */
    uint8_t stop_bits;            /* Stop bits (1 or 2) */
    uint8_t parity;               /* 0=none, 1=odd, 2=even */
} uart_config_t;

/* ============================================================================
   Function Prototypes
   ============================================================================ */

/**
 * @brief Initialize UART communication
 * 
 * @param config UART configuration structure
 * @return uart_status_t Status code
 */
uart_status_t uart_init(const uart_config_t *config);

/**
 * @brief Transmit a single byte via UART
 * 
 * @param byte Byte to transmit
 * @return uart_status_t Status code
 */
uart_status_t uart_send_byte(uint8_t byte);

/**
 * @brief Transmit multiple bytes via UART
 * 
 * @param buffer Buffer containing data to send
 * @param length Number of bytes to send
 * @return uart_status_t Status code
 */
uart_status_t uart_send_buffer(const uint8_t *buffer, uint16_t length);

/**
 * @brief Transmit a string via UART
 * 
 * @param str Null-terminated string to transmit
 * @return uart_status_t Status code
 */
uart_status_t uart_send_string(const char *str);

/**
 * @brief Receive a single byte via UART with timeout
 * 
 * @param byte Output byte
 * @param timeout_ms Timeout in milliseconds
 * @return uart_status_t Status code
 */
uart_status_t uart_recv_byte(uint8_t *byte, uint16_t timeout_ms);

/**
 * @brief Receive multiple bytes via UART with timeout
 * 
 * @param buffer Output buffer
 * @param length Number of bytes to receive
 * @param timeout_ms Timeout in milliseconds
 * @return uart_status_t Status code
 */
uart_status_t uart_recv_buffer(uint8_t *buffer, uint16_t length, uint16_t timeout_ms);

/**
 * @brief Read a complete line (HEX record) from UART
 * 
 * @param line Output buffer for line
 * @param max_length Maximum line length
 * @param timeout_ms Timeout in milliseconds
 * @return uart_status_t Status code
 * 
 * @note Waits for ':' start character and newline terminator
 */
uart_status_t uart_recv_line(char *line, uint16_t max_length, uint16_t timeout_ms);

/**
 * @brief Check if UART has data available
 * 
 * @return uint8_t 1 if data available, 0 if not
 */
uint8_t uart_is_ready(void);

/**
 * @brief Flush UART receive buffer
 */
void uart_flush_rx(void);

/**
 * @brief Send acknowledgment
 * 
 * @return uart_status_t Status code
 */
uart_status_t uart_send_ack(void);

/**
 * @brief Send negative acknowledgment
 * 
 * @return uart_status_t Status code
 */
uart_status_t uart_send_nack(void);

/**
 * @brief Get error message for status code
 * 
 * @param status Status code
 * @return const char* Error message string
 */
const char* uart_get_error_string(uart_status_t status);

#endif /* UART_COMM_H */
