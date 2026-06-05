/**
 * @file    mapping_table.h
 * @brief   Channel mapping table: the central RAM cache of EEPROM-persisted
 *          configuration for all 250 data channels.
 *
 * Thread safety: all read/write access must be bracketed by
 *   xSemaphoreTake(mutexMappingTable, ...)
 *   xSemaphoreGive(mutexMappingTable)
 *
 * The cache lives in mapping_cache_t (declared in shared_types.h).
 */

#ifndef MAPPING_TABLE_H
#define MAPPING_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "shared_types.h"
#include "project_config.h"
#include <stdbool.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  Global mapping cache (defined in mapping_table.c)                    */
/* ------------------------------------------------------------------ */
extern mapping_cache_t g_mapping_cache;

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise the mapping cache from EEPROM. If EEPROM is blank
 *         or corrupt, load factory defaults.
 *
 * @return true if a valid configuration was loaded.
 */
bool mapping_table_init(void);

/**
 * @brief  Get a pointer to channel config 'id' from the cache.
 *         Caller must hold the mapping table mutex.
 *
 * @param id  Channel ID (0..MAX_DATA_CHANNELS-1)
 * @return Pointer to channel_config_t, or NULL if id out of range.
 */
const channel_config_t *mapping_get_channel(uint16_t id);

/**
 * @brief  Update a single channel's configuration in the RAM cache.
 *         Marks the cache as dirty for deferred EEPROM write.
 *
 * @return true if the channel is valid and updated.
 */
bool mapping_update_channel(uint16_t id, const channel_config_t *cfg);

/**
 * @brief  Find channel by KNX group address.
 * @return Channel ID, or 0xFFFF if not found.
 */
uint16_t mapping_find_by_knx_addr(uint16_t group_addr);

/**
 * @brief  Find channel by Modbus register type + address.
 * @return Channel ID, or 0xFFFF if not found.
 */
uint16_t mapping_find_by_modbus_reg(mb_reg_type_t reg_type, uint16_t reg_addr);

/**
 * @brief  Mark the cache as dirty (needs EEPROM persistence).
 */
void mapping_mark_dirty(void);

/**
 * @brief  Commit the dirty cache to EEPROM. Called from NvramTask context.
 * @return true on success.
 */
bool mapping_commit_to_eeprom(void);

/**
 * @brief  Load factory defaults into the cache (used on first boot or reset).
 */
void mapping_load_factory_defaults(void);

#ifdef __cplusplus
}
#endif

#endif /* MAPPING_TABLE_H */
