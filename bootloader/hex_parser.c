/**
 * @file hex_parser.c
 * @brief Intel HEX File Parser Implementation
 * @author ORKTech
 * @date 2026-04-25
 */

#include "hex_parser.h"
#include "bootloader_config.h"
#include <ctype.h>
#include <string.h>

/* ============================================================================
   Private Function Prototypes
   ============================================================================ */

static uint8_t hex_char_to_nibble(char c);
static uint8_t hex_is_valid_hex_char(char c);

/* ============================================================================
   Public Function Implementations
   ============================================================================ */

void hex_parser_init(hex_parser_state_t *parser)
{
    if (parser == NULL) {
        return;
    }
    
    parser->extended_address = 0x00000000;
    parser->current_address = 0x00000000;
    parser->line_number = 0;
    parser->eof_received = 0;
}

hex_status_t hex_parse_record(const char *line, hex_record_t *record)
{
    uint8_t i;
    uint8_t sum = 0;
    
    if (line == NULL || record == NULL) {
        return HEX_INVALID_DATA;
    }
    
    /* Check start code */
    if (line[0] != ':') {
        return HEX_INVALID_START_CODE;
    }
    
    /* Parse byte count (bytes 1-2) */
    if (!hex_is_valid_hex_char(line[1]) || !hex_is_valid_hex_char(line[2])) {
        return HEX_INVALID_BYTE_COUNT;
    }
    record->byte_count = (hex_char_to_nibble(line[1]) << 4) | 
                         hex_char_to_nibble(line[2]);
    
    if (record->byte_count > 255) {
        return HEX_INVALID_BYTE_COUNT;
    }
    
    /* Parse address (bytes 3-6) */
    if (!hex_is_valid_hex_char(line[3]) || !hex_is_valid_hex_char(line[4]) ||
        !hex_is_valid_hex_char(line[5]) || !hex_is_valid_hex_char(line[6])) {
        return HEX_INVALID_ADDRESS;
    }
    record->address = (hex_char_to_nibble(line[3]) << 12) |
                      (hex_char_to_nibble(line[4]) << 8)  |
                      (hex_char_to_nibble(line[5]) << 4)  |
                      hex_char_to_nibble(line[6]);
    
    /* Parse record type (bytes 7-8) */
    if (!hex_is_valid_hex_char(line[7]) || !hex_is_valid_hex_char(line[8])) {
        return HEX_INVALID_RECORD_TYPE;
    }
    record->record_type = (hex_char_to_nibble(line[7]) << 4) | 
                          hex_char_to_nibble(line[8]);
    
    if (record->record_type > 5) {
        return HEX_INVALID_RECORD_TYPE;
    }
    
    /* Parse data bytes */
    for (i = 0; i < record->byte_count; i++) {
        uint8_t idx = 9 + (i * 2);
        
        if (!hex_is_valid_hex_char(line[idx]) || !hex_is_valid_hex_char(line[idx + 1])) {
            return HEX_INVALID_DATA;
        }
        
        record->data[i] = (hex_char_to_nibble(line[idx]) << 4) | 
                          hex_char_to_nibble(line[idx + 1]);
    }
    
    /* Parse checksum (last 2 bytes) */
    uint8_t checksum_idx = 9 + (record->byte_count * 2);
    if (!hex_is_valid_hex_char(line[checksum_idx]) || 
        !hex_is_valid_hex_char(line[checksum_idx + 1])) {
        return HEX_INVALID_CHECKSUM;
    }
    record->checksum = (hex_char_to_nibble(line[checksum_idx]) << 4) | 
                       hex_char_to_nibble(line[checksum_idx + 1]);
    
    /* Verify checksum */
    if (!hex_verify_checksum(record)) {
        return HEX_INVALID_CHECKSUM;
    }
    
    return HEX_OK;
}

uint8_t hex_calculate_checksum(const hex_record_t *record)
{
    uint8_t sum = 0;
    uint8_t i;
    
    if (record == NULL) {
        return 0;
    }
    
    sum += record->byte_count;
    sum += (record->address >> 8) & 0xFF;
    sum += record->address & 0xFF;
    sum += record->record_type;
    
    for (i = 0; i < record->byte_count; i++) {
        sum += record->data[i];
    }
    
    return (~sum + 1) & 0xFF;
}

uint8_t hex_verify_checksum(const hex_record_t *record)
{
    uint8_t calculated;
    
    if (record == NULL) {
        return 0;
    }
    
    calculated = hex_calculate_checksum(record);
    return (calculated == record->checksum) ? 1 : 0;
}

hex_status_t hex_ascii_to_byte(char hi, char lo, uint8_t *byte)
{
    if (byte == NULL) {
        return HEX_INVALID_DATA;
    }
    
    if (!hex_is_valid_hex_char(hi) || !hex_is_valid_hex_char(lo)) {
        return HEX_INVALID_DATA;
    }
    
    *byte = (hex_char_to_nibble(hi) << 4) | hex_char_to_nibble(lo);
    return HEX_OK;
}

hex_status_t hex_update_state(hex_parser_state_t *parser, const hex_record_t *record)
{
    if (parser == NULL || record == NULL) {
        return HEX_INVALID_DATA;
    }
    
    switch (record->record_type) {
        case HEX_DATA:
            /* Update current address */
            parser->current_address = (parser->extended_address << 16) | record->address;
            
            /* Validate address range for application space */
            if (parser->current_address < APPLICATION_START || 
                parser->current_address > APPLICATION_END) {
                return HEX_ADDRESS_OUT_OF_RANGE;
            }
            break;
            
        case HEX_EXTENDED_LINEAR:
            /* Update extended address (upper 16 bits) */
            if (record->byte_count >= 2) {
                parser->extended_address = (record->data[0] << 8) | record->data[1];
            }
            break;
            
        case HEX_END_OF_FILE:
            parser->eof_received = 1;
            break;
            
        case HEX_START_LINEAR:
            /* Start linear address - not used in bootloader context */
            break;
            
        default:
            return HEX_INVALID_RECORD_TYPE;
    }
    
    parser->line_number++;
    return HEX_OK;
}

const char* hex_get_error_string(hex_status_t status)
{
    const char *error_strings[] = {
        "No error",
        "Invalid start code",
        "Invalid byte count",
        "Invalid address",
        "Invalid record type",
        "Invalid checksum",
        "Invalid data",
        "Buffer overflow",
        "Address out of range"
    };
    
    if (status > 8) {
        return "Unknown error";
    }
    
    return error_strings[status];
}

/* ============================================================================
   Private Function Implementations
   ============================================================================ */

static uint8_t hex_is_valid_hex_char(char c)
{
    return (isxdigit((unsigned char)c) != 0) ? 1 : 0;
}

static uint8_t hex_char_to_nibble(char c)
{
    if (c >= '0' && c <= '9') {
        return (uint8_t)(c - '0');
    } else if (c >= 'A' && c <= 'F') {
        return (uint8_t)(c - 'A' + 10);
    } else if (c >= 'a' && c <= 'f') {
        return (uint8_t)(c - 'a' + 10);
    }
    return 0;
}
