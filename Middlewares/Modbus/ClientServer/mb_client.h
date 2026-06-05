/**
 * @file    mb_client.h
 * @brief   Modbus RTU Client (Master) engine: request dispatch, response handling.
 */

#ifndef MB_CLIENT_H
#define MB_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include "mb_frame.h"

typedef enum {
    MB_CLIENT_OK,
    MB_CLIENT_TIMEOUT,
    MB_CLIENT_CRC_ERROR,
    MB_CLIENT_EXCEPTION,
    MB_CLIENT_BUSY
} mb_client_status_t;

typedef struct {
    uint8_t  slave_addr;
    uint16_t timeout_ms;
    uint16_t retries;
} mb_client_config_t;

void mb_client_init(const mb_client_config_t *config);

mb_client_status_t mb_client_read_coils(uint8_t slave, uint16_t addr, uint16_t count,
                                        uint8_t *dest);

mb_client_status_t mb_client_read_holding_regs(uint8_t slave, uint16_t addr, uint16_t count,
                                               uint16_t *dest);

mb_client_status_t mb_client_write_single_coil(uint8_t slave, uint16_t addr, uint8_t value);

mb_client_status_t mb_client_write_single_reg(uint8_t slave, uint16_t addr, uint16_t value);

mb_client_status_t mb_client_write_multiple_regs(uint8_t slave, uint16_t addr, uint16_t count,
                                                 const uint16_t *values);

#endif
