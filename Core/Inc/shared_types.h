/**
 * @file    shared_types.h
 * @brief   Common type definitions, enumerations, and structures used across
 *          all modules of the KNX/Modbus Smart Bridge.
 *
 * @note    All enums carry explicit width (C99 standard packing not assumed).
 *          Structures mapped for EEPROM storage use __attribute__((packed))
 *          on GCC/ARMCC and are validated with static_assert on sizeof().
 */

#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "project_config.h"

/* ------------------------------------------------------------------ */
/*  Basic Value Types (KNX <-> Modbus representations)                  */
/* ------------------------------------------------------------------ */

/** KNX DPT (Data Point Type) identifiers – subset used in bridge */
typedef enum {
    DPT_SWITCH              = 1,   /* 1.001  – 1 bit   */
    DPT_BOOL                = 2,   /* 1.002  – 1 bit   */
    DPT_SCALING             = 3,   /* 5.001  – 8 bit   */
    DPT_PERCENT_U8          = 4,   /* 5.004  – 8 bit   */
    DPT_TEMPERATURE         = 5,   /* 9.001  – 16 bit  */
    DPT_HUMIDITY            = 6,   /* 9.007  – 16 bit  */
    DPT_LUX                 = 7,   /* 9.004  – 16 bit  */
    DPT_FLOAT16             = 8,   /* 9.xxx  – 16 bit FP */
    DPT_U16                 = 9,   /* 7.001  – 16 bit  */
    DPT_U32                 = 10,  /* 12.001 – 32 bit  */
    DPT_ALARM_INFO          = 11,  /* alarm flags */
    DPT_CHAR_ASCII          = 12,  /* character */
    DPT_DATE                = 13,
    DPT_TIME                = 14,
    DPT_COUNT               = 15
} knx_dpt_id_t;

/** Modbus register type */
typedef enum {
    MB_COIL                 = 0,
    MB_DISCRETE_INPUT       = 1,
    MB_HOLDING_REGISTER     = 2,
    MB_INPUT_REGISTER       = 3
} mb_reg_type_t;

/** Direction of data flow for a channel */
typedef enum {
    FLOW_BIDIRECTIONAL      = 0,
    FLOW_KNX_TO_MODBUS      = 1,
    FLOW_MODBUS_TO_KNX      = 2
} channel_flow_t;

/** Operational profile of a Virtual Holder room */
typedef enum {
    VH_STATE_VACANT          = 0,
    VH_STATE_GUEST_PRESENT   = 1,
    VH_STATE_HOUSEKEEPING    = 2,
    VH_STATE_MAINTENANCE     = 3,
    VH_STATE_UNEXPECTED      = 4,   /* Motion without valid door sequence */
    VH_STATE_DOOR_OPEN       = 5    /* Door ajar, awaiting PIR confirmation */
} vh_room_state_t;

/** Alarm severity levels carried per channel */
typedef enum {
    ALARM_NONE              = 0,
    ALARM_INFO              = 1,
    ALARM_WARNING           = 2,
    ALARM_CRITICAL          = 3
} alarm_severity_t;

/** Logic block operator types */
typedef enum {
    LOGIC_OP_AND            = 0,
    LOGIC_OP_OR             = 1,
    LOGIC_OP_NOT            = 2,
    LOGIC_OP_XOR            = 3,
    LOGIC_OP_GT             = 4,   /* Greater than */
    LOGIC_OP_LT             = 5,   /* Less than */
    LOGIC_OP_GE             = 6,
    LOGIC_OP_LE             = 7,
    LOGIC_OP_EQ             = 8,
    LOGIC_OP_NEQ            = 9,
    LOGIC_OP_ADD            = 10,
    LOGIC_OP_SUB            = 11,
    LOGIC_OP_MUL            = 12,
    LOGIC_OP_DIV            = 13,
    LOGIC_OP_IF_THEN_ELSE   = 14,
    LOGIC_OP_PI_CONTROL     = 15,
    LOGIC_OP_DEW_POINT      = 16
} logic_op_t;

/* ------------------------------------------------------------------ */
/*  EEPROM-Persistent Configuration Structures                          */
/* ------------------------------------------------------------------ */

#pragma pack(push, 1)

/** A single data channel (KNX Group Object <-> Modbus register mapping).
 *  Occupies 36 bytes per channel (packed).
 *  250 channels * 36 = 9000 bytes total.
 */
typedef struct {
    uint16_t channel_id;           /* 0..249 */
    uint8_t  active;               /* 0 = disabled, 1 = enabled */
    uint8_t  flow_direction;       /* channel_flow_t */
    /* KNX side */
    uint16_t knx_group_addr;       /* 0/1/2/... */
    uint8_t  knx_dpt;             /* knx_dpt_id_t */
    /* Modbus side */
    uint8_t  mb_reg_type;         /* mb_reg_type_t */
    uint16_t mb_reg_addr;         /* 0..65535 */
    /* Transformation */
    float    scale_factor;         /* 1.0f = pass-through */
    float    offset;               /* 0.0f = pass-through */
    float    value_min;            /* Clamp minimum */
    float    value_max;            /* Clamp maximum */
    /* Alarm */
    uint8_t  alarm_enabled;
    float    alarm_high;
    float    alarm_low;
    uint8_t  alarm_severity;       /* alarm_severity_t */
} channel_config_t;

/* Static compile-time size check */
_Static_assert(sizeof(channel_config_t) == 36U, "channel_config_t must be 36 bytes");

/* ------------------------------------------------------------------ */
/*  Logic Block Configuration (stored in EEPROM)                        */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t  op;                   /* logic_op_t */
    uint8_t  operand_count;        /* 1..5 */
    uint16_t source_channel_a;     /* Channel ID for operand A */
    uint16_t source_channel_b;     /* Channel ID for operand B (0xFFFF if unused) */
    uint16_t source_channel_c;     /* Channel ID for operand C (0xFFFF if unused) */
    float    constant_a;
    float    constant_b;
    uint16_t dest_channel;         /* Where to write the result */
    uint8_t  enabled;
    uint8_t  reserved[5];
} logic_block_config_t;

_Static_assert(sizeof(logic_block_config_t) == 24U, "logic_block_config_t size mismatch");

/* ------------------------------------------------------------------ */
/*  PI Controller Runtime State (RAM only)                              */
/* ------------------------------------------------------------------ */

typedef struct {
    float   kp;
    float   ki;
    float   setpoint;
    float   integral;
    float   prev_error;
    float   out_min;
    float   out_max;
    uint32_t last_sample_tick;
    uint8_t  active;
    uint8_t  reserved[3];
} pi_state_t;

/* ------------------------------------------------------------------ */
/*  Virtual Holder Runtime State (RAM only)                             */
/* ------------------------------------------------------------------ */

typedef struct {
    vh_room_state_t current_state;
    vh_room_state_t previous_state;
    uint8_t  door_is_open;
    uint8_t  pir_triggered;
    uint32_t door_open_timestamp;   /* Tick when door last transitioned open */
    uint32_t last_pir_timestamp;    /* Tick of last PIR activity */
    uint32_t state_entry_timestamp; /* Tick when current state was entered */
    uint8_t  unexpected_flag_cnt;
    uint8_t  profile;               /* VH_PROFILE_xxx */
    uint8_t  reserved[2];
} vh_state_t;

/* ------------------------------------------------------------------ */
/*  EEPROM Persistent Configuration Header                              */
/* ------------------------------------------------------------------ */

typedef struct {
    uint32_t magic;                /* EEPROM_CONFIG_MAGIC */
    uint32_t version;              /* EEPROM_CONFIG_VERSION */
    uint32_t crc32;
    uint16_t channel_count;
    uint16_t logic_block_count;
    uint32_t reserved[4];
} eeprom_config_header_t;

_Static_assert(sizeof(eeprom_config_header_t) == 32U, "eeprom_config_header_t size mismatch");

#pragma pack(pop)

/* ------------------------------------------------------------------ */
/*  Mapping Table Runtime Cache (RAM mirror of EEPROM config)            */
/* ------------------------------------------------------------------ */

typedef struct {
    channel_config_t  channels[MAX_DATA_CHANNELS];
    logic_block_config_t logic_blocks[MAX_LOGIC_BLOCKS];
    eeprom_config_header_t header;
    uint8_t dirty;                       /* Set to 1 when cache != EEPROM */
} mapping_cache_t;

/* ------------------------------------------------------------------ */
/*  Unified Data Value (used across the broker)                          */
/* ------------------------------------------------------------------ */

typedef union {
    uint8_t   u8;
    int8_t    i8;
    uint16_t  u16;
    int16_t   i16;
    uint32_t  u32;
    int32_t   i32;
    float     f32;
    uint8_t   raw[4];
} data_value_t;

#ifdef __cplusplus
}
#endif

#endif /* SHARED_TYPES_H */
