/**
 * @file    project_config.h
 * @brief   Global project definitions, build-time constants, and feature toggles
 *          for the STM32 KNX/Modbus Smart Bridge.
 *
 * @note    All magic numbers must be resolved to named constants per MISRA C 2012 Dir 4.3.
 */

#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  System Identification                                              */
/* ------------------------------------------------------------------ */
#define DEVICE_MANUFACTURER_ID      0x00FAU
#define DEVICE_TYPE_ID              0x1000U
#define FIRMWARE_MAJOR              1U
#define FIRMWARE_MINOR              0U
#define FIRMWARE_PATCH              0U

/* ------------------------------------------------------------------ */
/*  KNX Stack Configuration                                            */
/* ------------------------------------------------------------------ */
#define KNX_TPUART_BAUDRATE         9600U
#define KNX_MAX_TELEGRAM_LEN        23U
#define KNX_MAX_APDU_LEN            15U
#define KNX_GROUP_OBJECT_COUNT      250U
#define KNX_INDIVIDUAL_ADDRESS      0x1101U          /* 1.1.001 default */
#define KNX_DATA_SECURE_ENABLED     1U               /* Set to 0 if no secure support */

/* ------------------------------------------------------------------ */
/*  Modbus Stack Configuration                                         */
/* ------------------------------------------------------------------ */
#define MODBUS_RTU_BAUDRATE         38400U
#define MODBUS_SLAVE_ADDRESS        1U
#define MODBUS_MAX_FRAME_LEN        256U
#define MODBUS_COIL_COUNT           250U
#define MODBUS_HOLDING_REG_COUNT    250U
#define MODBUS_INPUT_REG_COUNT      64U
#define MODBUS_CHAR_TIMEOUT_T15_MS  3U               /* 1.5 char times @ 38400 bps */
#define MODBUS_FRAME_TIMEOUT_T35_MS 5U               /* 3.5 char times @ 38400 bps */

/* ------------------------------------------------------------------ */
/*  Data Channel / Mapping Limits                                      */
/* ------------------------------------------------------------------ */
#define MAX_DATA_CHANNELS           250U
#define MAX_LOGIC_BLOCKS            50U
#define LOGIC_EXPRESSION_MAX_TOKENS 32U

/* ------------------------------------------------------------------ */
/*  RS485 Direction Control Timing (microseconds)                       */
/* ------------------------------------------------------------------ */
#define RS485_TX_PREDELAY_US        50U
#define RS485_TX_POSTDELAY_US       50U

/* ------------------------------------------------------------------ */
/*  EEPROM / NVRAM Layout                                              */
/* ------------------------------------------------------------------ */
#define EEPROM_PAGE_SIZE            256U
#define EEPROM_TOTAL_SIZE           65536U           /* 64 KiB typical */
#define EEPROM_CONFIG_MAGIC         0xBEEFCAFEUL
#define EEPROM_CONFIG_VERSION       1U
#define EEPROM_WRITE_RETRY          3U

/* ------------------------------------------------------------------ */
/*  Logic Engine / Math                                                */
/* ------------------------------------------------------------------ */
#define PI_CONTROLLER_DEFAULT_KP    1.0f
#define PI_CONTROLLER_DEFAULT_KI    0.1f
#define PI_CONTROLLER_OUT_MIN       0.0f
#define PI_CONTROLLER_OUT_MAX       100.0f
#define DEW_POINT_MAGNUS_A          17.27f
#define DEW_POINT_MAGNUS_B          237.7f

/* ------------------------------------------------------------------ */
/*  Virtual Holder State Machine                                       */
/* ------------------------------------------------------------------ */
#define VH_DOOR_DEBOUNCE_MS         50U
#define VH_PIR_TIMEOUT_S            30U
#define VH_DOOR_OPEN_TIMEOUT_S      10U
#define VH_PROFILE_GUEST            0x01U
#define VH_PROFILE_HOUSEKEEPING     0x02U
#define VH_PROFILE_MAINTENANCE      0x03U
#define VH_PROFILE_UNKNOWN          0xFFU

/* ------------------------------------------------------------------ */
/*  Watchdog & Diagnostics                                             */
/* ------------------------------------------------------------------ */
#define WATCHDOG_REFRESH_MS         500U
#define DIAG_HEARTBEAT_MS           1000U

/* ------------------------------------------------------------------ */
/*  Assertion & Error Handling                                          */
/* ------------------------------------------------------------------ */
#ifndef BRIDGE_ASSERT
  #define BRIDGE_ASSERT(x)  do { if (!(x)) { bridge_assert_failed(__FILE__, __LINE__); } } while(0U)
#endif

void bridge_assert_failed(const char *file, int line);

#ifdef __cplusplus
}
#endif

#endif /* PROJECT_CONFIG_H */
