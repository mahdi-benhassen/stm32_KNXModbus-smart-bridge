/**
 * @file    ets_parser.h
 * @brief   ETS (Engineering Tool Software) application parameter parser.
 *
 * Parses the KNX application program data structure as defined in the
 * ETS project export file (.knxproj) or directly from the ETS memory
 * layout when loaded over the bus.
 */

#ifndef ETS_PARSER_H
#define ETS_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#define ETS_MAX_PARAM_BYTES     256U

typedef enum {
    ETS_PARAM_TYPE_U8,
    ETS_PARAM_TYPE_U16,
    ETS_PARAM_TYPE_U32,
    ETS_PARAM_TYPE_FLOAT,
    ETS_PARAM_TYPE_STRING,
    ETS_PARAM_TYPE_ENUM
} ets_param_type_t;

typedef struct {
    uint16_t        id;
    const char      *name;
    ets_param_type_t type;
    uint16_t        offset;
    uint8_t         bit_pos;
    uint8_t         bit_len;
    float           min;
    float           max;
    float           default_val;
} ets_param_descriptor_t;

bool ets_parser_init(const uint8_t *app_data, uint16_t app_len);

bool ets_param_get_uint8(uint16_t param_id, uint8_t *value);

bool ets_param_get_uint16(uint16_t param_id, uint16_t *value);

bool ets_param_get_float(uint16_t param_id, float *value);

bool ets_param_get_enum(uint16_t param_id, uint8_t *value);

bool ets_import_to_mapping_table(void);

#endif
