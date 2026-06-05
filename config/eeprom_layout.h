/**
 * @file    eeprom_layout.h
 * @brief   Persistent storage layout for the 64 KiB external EEPROM.
 *
 * The EEPROM is logically partitioned into three regions:
 *
 *    Offset  | Size   | Region
 *   ---------|--------|-----------------------------------------------
 *    0x0000  |   32 B | Configuration Header  (magic, version, CRC)
 *    0x0020  | 9000 B | Channel Table          (250 × 36 bytes = 9000)
 *    0x2400  | 1200 B | Logic Block Table      (50 × 24 bytes = 1200)
 *    0x28B0  |  ...   | Reserved / future
 *
 *  Writes are performed through the NvramTask using an on-demand
 *  dirty-flag approach to minimise EEPROM wear.
 */

#ifndef EEPROM_LAYOUT_H
#define EEPROM_LAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "project_config.h"
#include "shared_types.h"

/* ------------------------------------------------------------------ */
/*  Absolute EEPROM Offsets                                            */
/* ------------------------------------------------------------------ */
#define EEPROM_OFFSET_HEADER           0x0000U
#define EEPROM_OFFSET_CHANNELS         0x0020U
#define EEPROM_OFFSET_LOGIC_BLOCKS     0x2400U
#define EEPROM_OFFSET_END              0x28B0U   /* Next free area */

/* ------------------------------------------------------------------ */
/*  Region Sizes                                                        */
/* ------------------------------------------------------------------ */
#define EEPROM_HEADER_SIZE             sizeof(eeprom_config_header_t)   /* 32 bytes */
#define EEPROM_CHANNEL_SIZE            MAX_DATA_CHANNELS * sizeof(channel_config_t)
#define EEPROM_LOGIC_BLOCK_SIZE        MAX_LOGIC_BLOCKS * sizeof(logic_block_config_t)

/* Compile-time guard: ensure everything fits */
_Static_assert((EEPROM_OFFSET_CHANNELS + EEPROM_CHANNEL_SIZE) <= EEPROM_OFFSET_LOGIC_BLOCKS,
               "Channel table overflows into logic block region");
_Static_assert((EEPROM_OFFSET_LOGIC_BLOCKS + EEPROM_LOGIC_BLOCK_SIZE) <= EEPROM_TOTAL_SIZE,
               "Logic block table exceeds EEPROM capacity");

/* ------------------------------------------------------------------ */
/*  CRC-32 (Ethernet polynomial) utility                                 */
/* ------------------------------------------------------------------ */
uint32_t eeprom_crc32(const uint8_t *data, size_t len);

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief  Load configuration from EEPROM into the RAM mapping cache.
 *         Validates magic number and CRC. Returns false if corrupt.
 */
bool eeprom_load_config(mapping_cache_t *cache);

/**
 * @brief  Persist the RAM mapping cache to EEPROM.
 *         Must be called from NvramTask context.
 */
bool eeprom_save_config(const mapping_cache_t *cache);

/**
 * @brief  Erase all configuration from EEPROM (factory reset).
 */
bool eeprom_factory_reset(void);

/**
 * @brief  Write a single channel to EEPROM (used for incremental updates).
 */
bool eeprom_write_channel(uint16_t channel_id, const channel_config_t *cfg);

/**
 * @brief  Write a single logic block to EEPROM.
 */
bool eeprom_write_logic_block(uint16_t block_id, const logic_block_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* EEPROM_LAYOUT_H */
