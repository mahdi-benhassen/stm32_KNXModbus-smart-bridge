/**
 * @file    knx_group_objects.h
 * @brief   KNX group object table: 250 objects mapped to channels.
 */

#ifndef KNX_GROUP_OBJECTS_H
#define KNX_GROUP_OBJECTS_H

#include "project_config.h"
#include "shared_types.h"
#include <stdint.h>

typedef struct {
    uint16_t go_number;
    uint16_t group_addr;
    uint8_t  dpt;
    uint8_t  flags;         /* Bit0: communication, Bit1: read, Bit2: write, Bit3: transmit */
    uint8_t  value[4];
    uint8_t  value_len;
} knx_group_object_t;

void knx_go_init(void);

const knx_group_object_t *knx_go_get(uint16_t index);

bool knx_go_update_value(uint16_t go_number, const uint8_t *value, uint8_t len);

bool knx_go_read_value(uint16_t go_number, uint8_t *value, uint8_t *len);

#endif
