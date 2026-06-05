/**
 * @file    mb_rtu.h
 * @brief   Modbus RTU stack: frame assembly, CRC, timeout management.
 */

#ifndef MB_RTU_H
#define MB_RTU_H

#include <stdint.h>
#include <stdbool.h>

#define MB_RTU_MAX_FRAME_LEN    256U
#define MB_RTU_BROADCAST_ADDR   0U

uint16_t mb_rtu_crc16(const uint8_t *data, uint16_t len);

bool mb_rtu_validate_crc(const uint8_t *frame, uint16_t len);

uint8_t mb_rtu_build_request(uint8_t slave_addr, uint8_t func_code,
                             uint16_t start_addr, uint16_t quantity,
                             uint8_t *buffer, uint16_t max_len);

uint8_t mb_rtu_build_response(uint8_t slave_addr, uint8_t func_code,
                              uint8_t byte_count, const uint8_t *data,
                              uint8_t *buffer, uint16_t max_len);

uint8_t mb_rtu_build_exception(uint8_t slave_addr, uint8_t func_code,
                               uint8_t exception_code,
                               uint8_t *buffer, uint16_t max_len);

#endif
