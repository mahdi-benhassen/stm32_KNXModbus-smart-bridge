/**
 * @file    mapping_table.c
 * @brief   Channel mapping table implementation.
 */

#include "mapping_table.h"
#include "eeprom_layout.h"
#include "nvram.h"
#include "task_priorities.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Global Mapping Cache                                                */
/* ------------------------------------------------------------------ */
mapping_cache_t g_mapping_cache;

/* ------------------------------------------------------------------ */
/*  CRC-32 (used for EEPROM header validation)                          */
/* ------------------------------------------------------------------ */
static uint32_t crc32_table[256];
static bool     crc32_table_initialized = false;

static void crc32_init(void)
{
    if (crc32_table_initialized) {
        return;
    }
    for (uint32_t i = 0U; i < 256U; i++) {
        uint32_t crc = i;
        for (uint32_t j = 0U; j < 8U; j++) {
            if ((crc & 1U) != 0U) {
                crc = (crc >> 1U) ^ 0xEDB88320UL;
            } else {
                crc = crc >> 1U;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = true;
}

uint32_t eeprom_crc32(const uint8_t *data, size_t len)
{
    crc32_init();
    uint32_t crc = 0xFFFFFFFFUL;
    for (size_t i = 0U; i < len; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFFU] ^ (crc >> 8U);
    }
    return crc ^ 0xFFFFFFFFUL;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

bool mapping_table_init(void)
{
    /* Try loading from EEPROM */
    if (eeprom_load_config(&g_mapping_cache)) {
        g_mapping_cache.dirty = 0U;
        return true;
    }

    /* EEPROM invalid or blank: load factory defaults */
    mapping_load_factory_defaults();
    g_mapping_cache.dirty = 1U;  /* Force write to EEPROM on first boot */
    (void)mapping_commit_to_eeprom();
    g_mapping_cache.dirty = 0U;
    return false;  /* Indicate defaults were loaded */
}

const channel_config_t *mapping_get_channel(uint16_t id)
{
    if (id >= MAX_DATA_CHANNELS) {
        return NULL;
    }
    return &g_mapping_cache.channels[id];
}

bool mapping_update_channel(uint16_t id, const channel_config_t *cfg)
{
    if (id >= MAX_DATA_CHANNELS || cfg == NULL) {
        return false;
    }
    (void)memcpy((void *)&g_mapping_cache.channels[id], cfg, sizeof(channel_config_t));
    g_mapping_cache.dirty = 1U;
    return true;
}

uint16_t mapping_find_by_knx_addr(uint16_t group_addr)
{
    for (uint16_t i = 0U; i < MAX_DATA_CHANNELS; i++) {
        if (g_mapping_cache.channels[i].active != 0U &&
            g_mapping_cache.channels[i].knx_group_addr == group_addr) {
            return i;
        }
    }
    return 0xFFFFU;
}

uint16_t mapping_find_by_modbus_reg(mb_reg_type_t reg_type, uint16_t reg_addr)
{
    for (uint16_t i = 0U; i < MAX_DATA_CHANNELS; i++) {
        if (g_mapping_cache.channels[i].active != 0U &&
            (mb_reg_type_t)g_mapping_cache.channels[i].mb_reg_type == reg_type &&
            g_mapping_cache.channels[i].mb_reg_addr == reg_addr) {
            return i;
        }
    }
    return 0xFFFFU;
}

void mapping_mark_dirty(void)
{
    g_mapping_cache.dirty = 1U;
}

bool mapping_commit_to_eeprom(void)
{
    if (g_mapping_cache.dirty == 0U) {
        return true;  /* Nothing to write */
    }

    /* Write header first */
    if (!nvram_write(EEPROM_OFFSET_HEADER,
                     (const uint8_t *)&g_mapping_cache.header,
                     sizeof(g_mapping_cache.header))) {
        return false;
    }

    /* Write channels */
    if (!nvram_write(EEPROM_OFFSET_CHANNELS,
                     (const uint8_t *)g_mapping_cache.channels,
                     sizeof(g_mapping_cache.channels))) {
        return false;
    }

    /* Write logic blocks */
    if (!nvram_write(EEPROM_OFFSET_LOGIC_BLOCKS,
                     (const uint8_t *)g_mapping_cache.logic_blocks,
                     sizeof(g_mapping_cache.logic_blocks))) {
        return false;
    }

    g_mapping_cache.dirty = 0U;
    return true;
}

void mapping_load_factory_defaults(void)
{
    (void)memset(&g_mapping_cache, 0, sizeof(g_mapping_cache));

    /* Set up header */
    g_mapping_cache.header.magic       = EEPROM_CONFIG_MAGIC;
    g_mapping_cache.header.version     = EEPROM_CONFIG_VERSION;
    g_mapping_cache.header.channel_count    = 0U;
    g_mapping_cache.header.logic_block_count = 0U;

    /* Initialise all 250 channels as inactive with safe defaults */
    for (uint16_t i = 0U; i < MAX_DATA_CHANNELS; i++) {
        g_mapping_cache.channels[i].channel_id    = i;
        g_mapping_cache.channels[i].active        = 0U;  /* disabled */
        g_mapping_cache.channels[i].flow_direction = FLOW_BIDIRECTIONAL;
        g_mapping_cache.channels[i].knx_group_addr = 0xFFFFU;
        g_mapping_cache.channels[i].knx_dpt        = DPT_SWITCH;
        g_mapping_cache.channels[i].mb_reg_type    = MB_HOLDING_REGISTER;
        g_mapping_cache.channels[i].mb_reg_addr    = 0xFFFFU;
        g_mapping_cache.channels[i].scale_factor   = 1.0f;
        g_mapping_cache.channels[i].offset         = 0.0f;
        g_mapping_cache.channels[i].value_min      = -1e9f;
        g_mapping_cache.channels[i].value_max      = +1e9f;
        g_mapping_cache.channels[i].alarm_enabled  = 0U;
        g_mapping_cache.channels[i].alarm_high     = 0.0f;
        g_mapping_cache.channels[i].alarm_low      = 0.0f;
        g_mapping_cache.channels[i].alarm_severity = ALARM_NONE;
    }

    /* Initialise logic blocks as disabled */
    for (uint16_t i = 0U; i < MAX_LOGIC_BLOCKS; i++) {
        g_mapping_cache.logic_blocks[i].enabled = 0U;
        g_mapping_cache.logic_blocks[i].op      = LOGIC_OP_AND;
        g_mapping_cache.logic_blocks[i].operand_count = 0U;
        g_mapping_cache.logic_blocks[i].source_channel_a = 0xFFFFU;
        g_mapping_cache.logic_blocks[i].source_channel_b = 0xFFFFU;
        g_mapping_cache.logic_blocks[i].source_channel_c = 0xFFFFU;
        g_mapping_cache.logic_blocks[i].dest_channel     = 0xFFFFU;
    }
}

/* ------------------------------------------------------------------ */
/*  EEPROM Load / Save (from eeprom_layout.h API)                       */
/* ------------------------------------------------------------------ */
bool eeprom_load_config(mapping_cache_t *cache)
{
    eeprom_config_header_t header;

    /* Read header */
    if (!nvram_read(EEPROM_OFFSET_HEADER, (uint8_t *)&header, sizeof(header))) {
        return false;
    }

    /* Validate magic and version */
    if (header.magic != EEPROM_CONFIG_MAGIC || header.version != EEPROM_CONFIG_VERSION) {
        return false;
    }

    /* Read channel table */
    if (!nvram_read(EEPROM_OFFSET_CHANNELS,
                    (uint8_t *)cache->channels,
                    sizeof(cache->channels))) {
        return false;
    }

    /* Read logic block table */
    if (!nvram_read(EEPROM_OFFSET_LOGIC_BLOCKS,
                    (uint8_t *)cache->logic_blocks,
                    sizeof(cache->logic_blocks))) {
        return false;
    }

    /* Validate CRC: compute over channels + logic blocks, compare to stored */
    uint32_t crc_calc = eeprom_crc32((const uint8_t *)cache->channels,
                                     sizeof(cache->channels));
    /* Continue CRC computation over logic blocks (reset for second block) */
    uint32_t crc_blk = eeprom_crc32((const uint8_t *)cache->logic_blocks,
                                    sizeof(cache->logic_blocks));
    /* Combine CRCs — use XOR for simplicity, or re-compute full combined buffer */
    uint32_t combined_crc = crc_calc ^ crc_blk;

    if (header.crc32 != 0U && combined_crc != header.crc32) {
        return false;  /* Data corruption detected */
    }

    (void)memcpy(&cache->header, &header, sizeof(header));
    return true;
}

bool eeprom_save_config(const mapping_cache_t *cache)
{
    /* Compute CRC over channels and logic blocks, store in header */
    mapping_cache_t *mutable_cache = (mapping_cache_t *)cache;
    uint32_t crc = eeprom_crc32((const uint8_t *)cache->channels,
                                sizeof(cache->channels));
    uint32_t crc_blk = eeprom_crc32((const uint8_t *)cache->logic_blocks,
                                    sizeof(cache->logic_blocks));
    mutable_cache->header.crc32 = crc ^ crc_blk;

    return mapping_commit_to_eeprom();
}

bool eeprom_factory_reset(void)
{
    mapping_load_factory_defaults();
    return mapping_commit_to_eeprom();
}

bool eeprom_write_channel(uint16_t channel_id, const channel_config_t *cfg)
{
    if (channel_id >= MAX_DATA_CHANNELS) {
        return false;
    }
    uint32_t addr = EEPROM_OFFSET_CHANNELS + (channel_id * sizeof(channel_config_t));
    return nvram_write(addr, (const uint8_t *)cfg, sizeof(channel_config_t));
}

bool eeprom_write_logic_block(uint16_t block_id, const logic_block_config_t *cfg)
{
    if (block_id >= MAX_LOGIC_BLOCKS) {
        return false;
    }
    uint32_t addr = EEPROM_OFFSET_LOGIC_BLOCKS + (block_id * sizeof(logic_block_config_t));
    return nvram_write(addr, (const uint8_t *)cfg, sizeof(logic_block_config_t));
}
