/**
 * @file    mb_frame.h
 * @brief   Modbus frame parser and helper utilities.
 */

#ifndef MB_FRAME_H
#define MB_FRAME_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t  slave_addr;
    uint8_t  func_code;
    uint8_t  exception_code;
    uint16_t start_addr;
    uint16_t quantity;
    uint8_t  byte_count;
    uint8_t  data[252];
    uint16_t data_len;
    bool     is_broadcast;
    bool     is_exception;
} mb_frame_parsed_t;

bool mb_frame_parse(const uint8_t *raw, uint16_t raw_len, mb_frame_parsed_t *out);

const char *mb_frame_exception_string(uint8_t exception_code);

#endif
