/**
 * @file hex_parser.h
 * @brief Intel HEX File Parser for Bootloader
 * @author ORKTech
 * @date 2026-04-25
 * 
 * Parses Intel HEX format firmware files for programming into PIC18F2520
 */

#ifndef HEX_PARSER_H
#define HEX_PARSER_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
   HEX Record Types
   ============================================================================ */

typedef enum {
    HEX_DATA                = 0x00,  /* Data record */
    HEX_END_OF_FILE         = 0x01,  /* End of file */
    HEX_EXTENDED_LINEAR     = 0x04,  /* Extended linear address */
    HEX_START_LINEAR        = 0x05   /* Start linear address */
} hex_record_type_t;

/* ============================================================================
   HEX Parser Status Codes
   ============================================================================ */

typedef enum {
    HEX_OK                  = 0,
    HEX_INVALID_START_CODE  = 1,     /* Missing ':' at start */
    HEX_INVALID_BYTE_COUNT  = 2,     /* Invalid byte count field */
    HEX_INVALID_ADDRESS     = 3,     /* Invalid address field */
    HEX_INVALID_RECORD_TYPE = 4,     /* Invalid record type */
    HEX_INVALID_CHECKSUM    = 5,     /* Checksum mismatch */
    HEX_INVALID_DATA        = 6,     /* Invalid data bytes */
    HEX_BUFFER_OVERFLOW     = 7,     /* Data buffer overflow */
    HEX_ADDRESS_OUT_OF_RANGE= 8      /* Address outside valid range */
} hex_status_t;

/* ============================================================================
   HEX Record Structure
   ============================================================================ */

typedef struct {
    uint8_t  byte_count;              /* Number of data bytes */
    uint16_t address;                 /* 16-bit address */
    uint8_t  record_type;             /* Record type */
    uint8_t  data[256];               /* Data bytes */
    uint8_t  checksum;                /* Checksum byte */
} hex_record_t;

/* ============================================================================
   HEX Parser State
   ============================================================================ */

typedef struct {
    uint32_t extended_address;        /* Extended linear address (upper 16 bits) */
    uint32_t current_address;         /* Current absolute address */
    uint32_t line_number;             /* Current line number */
    uint8_t  eof_received;            /* EOF marker received flag */
} hex_parser_state_t;

/* ============================================================================
   Function Prototypes
   ============================================================================ */

/**
 * @brief Initialize HEX parser state
 * @param parser Parser state structure
 */
void hex_parser_init(hex_parser_state_t *parser);

/**
 * @brief Parse a single HEX record line
 * 
 * @param line Input HEX record string (must start with ':')
 * @param record Output parsed record structure
 * @return hex_status_t Status code
 */
hex_status_t hex_parse_record(const char *line, hex_record_t *record);

/**
 * @brief Calculate checksum for a HEX record
 * 
 * @param record HEX record to verify
 * @return uint8_t Calculated checksum
 */
uint8_t hex_calculate_checksum(const hex_record_t *record);

/**
 * @brief Verify HEX record checksum
 * 
 * @param record HEX record to verify
 * @return uint8_t 1 if valid, 0 if invalid
 */
uint8_t hex_verify_checksum(const hex_record_t *record);

/**
 * @brief Convert ASCII hex characters to byte
 * 
 * @param hi High nibble ASCII character
 * @param lo Low nibble ASCII character
 * @param byte Output byte value
 * @return hex_status_t Status code
 */
hex_status_t hex_ascii_to_byte(char hi, char lo, uint8_t *byte);

/**
 * @brief Update parser state with record information
 * 
 * @param parser Parser state
 * @param record HEX record
 * @return hex_status_t Status code
 */
hex_status_t hex_update_state(hex_parser_state_t *parser, const hex_record_t *record);

/**
 * @brief Get error message for status code
 * 
 * @param status Status code
 * @return const char* Error message string
 */
const char* hex_get_error_string(hex_status_t status);

#endif /* HEX_PARSER_H */
