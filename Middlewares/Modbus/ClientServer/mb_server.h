/**
 * @file    mb_server.h
 * @brief   Modbus RTU Server (Slave) engine: register map, function code dispatch.
 */

#ifndef MB_SERVER_H
#define MB_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include "mb_frame.h"

typedef bool (*mb_server_read_coil_cb)(uint16_t addr, uint8_t *value);
typedef bool (*mb_server_write_coil_cb)(uint16_t addr, uint8_t value);
typedef bool (*mb_server_read_reg_cb)(uint16_t addr, uint16_t *value);
typedef bool (*mb_server_write_reg_cb)(uint16_t addr, uint16_t value);

typedef struct {
    mb_server_read_coil_cb    read_coil;
    mb_server_write_coil_cb   write_coil;
    mb_server_read_reg_cb     read_holding_reg;
    mb_server_write_reg_cb    write_holding_reg;
    mb_server_read_reg_cb     read_input_reg;
    uint8_t                   slave_address;
} mb_server_config_t;

void mb_server_init(const mb_server_config_t *config);

bool mb_server_process_frame(const uint8_t *raw, uint16_t raw_len,
                             uint8_t *response, uint16_t *resp_len);

#endif
