/**
 * @file uart_comm.c
 * @brief UART Communication Implementation
 * @author ORKTech
 * @date 2026-04-25
 */

#include "uart_comm.h"
#include "bootloader_config.h"
#include <string.h>

/* ============================================================================
   UART Module State
   ============================================================================ */

static struct {
    uint8_t initialized;
    uart_config_t config;
} uart_state = {
    .initialized = 0,
    .config = {
        .baud_rate = 9600,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0
    }
};

/* ============================================================================
   Private Function Prototypes
   ============================================================================ */

static uint16_t uart_get_timeout_ticks(uint16_t timeout_ms);
static uint8_t uart_is_tx_ready(void);
static uint8_t uart_is_rx_ready(void);

/* ============================================================================
   Public Function Implementations
   ============================================================================ */

uart_status_t uart_init(const uart_config_t *config)
{
    if (config == NULL) {
        return UART_NOT_READY;
    }
    
    /* Copy configuration */
    uart_state.config = *config;
    
    /* In real PIC18F2520 implementation, would:
     * 1. Set TXSTA register (TX enable, async mode)
     * 2. Set RCSTA register (RX enable)
     * 3. Calculate and set SPBRG register based on baud rate
     *    SPBRG = (Fosc / (16 * Baud)) - 1
     * 4. Enable interrupts if needed
     * 5. Set TRISC bits for TX/RX pins
     */
    
    uart_state.initialized = 1;
    return UART_OK;
}

uart_status_t uart_send_byte(uint8_t byte)
{
    if (!uart_state.initialized) {
        return UART_NOT_READY;
    }
    
    if (!uart_is_tx_ready()) {
        return UART_TX_ERROR;
    }
    
    /* In real implementation, would:
     * Write byte to TXREG register
     * Wait for TXIF flag (or return if non-blocking)
     */
    
    return UART_OK;
}

uart_status_t uart_send_buffer(const uint8_t *buffer, uint16_t length)
{
    uint16_t i;
    uart_status_t status;
    
    if (buffer == NULL || length == 0) {
        return UART_NOT_READY;
    }
    
    for (i = 0; i < length; i++) {
        status = uart_send_byte(buffer[i]);
        if (status != UART_OK) {
            return status;
        }
    }
    
    return UART_OK;
}

uart_status_t uart_send_string(const char *str)
{
    if (str == NULL) {
        return UART_NOT_READY;
    }
    
    return uart_send_buffer((const uint8_t *)str, (uint16_t)strlen(str));
}

uart_status_t uart_recv_byte(uint8_t *byte, uint16_t timeout_ms)
{
    uint16_t timeout_ticks;
    
    if (byte == NULL || !uart_state.initialized) {
        return UART_NOT_READY;
    }
    
    timeout_ticks = uart_get_timeout_ticks(timeout_ms);
    
    /* Wait for receive data with timeout */
    while (timeout_ticks > 0) {
        if (uart_is_rx_ready()) {
            /* In real implementation, would read from RCREG */
            *byte = 0x00;
            return UART_OK;
        }
        timeout_ticks--;
    }
    
    return UART_RX_TIMEOUT;
}

uart_status_t uart_recv_buffer(uint8_t *buffer, uint16_t length, uint16_t timeout_ms)
{
    uint16_t i;
    uart_status_t status;
    
    if (buffer == NULL || length == 0) {
        return UART_NOT_READY;
    }
    
    for (i = 0; i < length; i++) {
        status = uart_recv_byte(&buffer[i], timeout_ms);
        if (status != UART_OK) {
            return status;
        }
    }
    
    return UART_OK;
}

uart_status_t uart_recv_line(char *line, uint16_t max_length, uint16_t timeout_ms)
{
    uint8_t byte;
    uint16_t index = 0;
    uart_status_t status;
    uint16_t remaining_timeout = timeout_ms;
    
    if (line == NULL || max_length == 0) {
        return UART_NOT_READY;
    }
    
    /* Wait for start character ':' */
    do {
        status = uart_recv_byte(&byte, remaining_timeout);
        if (status != UART_OK) {
            return status;
        }
    } while (byte != ':');
    
    /* Store ':' */
    line[index++] = (char)byte;
    
    /* Read until newline or buffer full */
    while (index < max_length) {
        status = uart_recv_byte(&byte, timeout_ms);
        if (status != UART_OK) {
            return status;
        }
        
        line[index++] = (char)byte;
        
        /* Check for line terminator */
        if (byte == '\n' || byte == '\r') {
            break;
        }
    }
    
    /* Null-terminate if there's space */
    if (index < max_length) {
        line[index] = '\0';
    } else {
        return UART_RX_OVERFLOW;
    }
    
    return UART_OK;
}

uint8_t uart_is_ready(void)
{
    return uart_state.initialized ? uart_is_rx_ready() : 0;
}

void uart_flush_rx(void)
{
    uint8_t dummy;
    
    /* Flush any pending bytes */
    while (uart_is_rx_ready()) {
        uart_recv_byte(&dummy, 0);
    }
}

uart_status_t uart_send_ack(void)
{
    /* Send ':' as acknowledgment */
    return uart_send_byte(CMD_ACK);
}

uart_status_t uart_send_nack(void)
{
    /* Send NACK command */
    return uart_send_byte(CMD_NACK);
}

const char* uart_get_error_string(uart_status_t status)
{
    const char *error_strings[] = {
        "No error",
        "Transmit error",
        "Receive timeout",
        "Receive overflow",
        "Frame error",
        "Parity error",
        "Not ready"
    };
    
    if (status > 6) {
        return "Unknown error";
    }
    
    return error_strings[status];
}

/* ============================================================================
   Private Function Implementations
   ============================================================================ */

static uint16_t uart_get_timeout_ticks(uint16_t timeout_ms)
{
    /* Convert milliseconds to timeout ticks
     * Depends on system clock frequency
     * For simulation, use 1 tick = 1ms
     */
    return timeout_ms;
}

static uint8_t uart_is_tx_ready(void)
{
    /* In real implementation, would check TXIF flag in PIR1 */
    return 1;
}

static uint8_t uart_is_rx_ready(void)
{
    /* In real implementation, would check RCIF flag in PIR1 */
    return 0;
}
