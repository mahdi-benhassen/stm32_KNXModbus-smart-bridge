/**
 * @file    nvram.c
 * @brief   I2C EEPROM driver implementation (AT24C512 or compatible).
 *
 * Page-write boundary handling: the AT24C512 uses 128-byte pages.
 * Writes that cross a page boundary are split automatically.
 */

#include "nvram.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Private Constants                                                   */
/* ------------------------------------------------------------------ */
#define EEPROM_PAGE_SIZE_DEVICE  128U          /* AT24C512 page size */
#define EEPROM_WRITE_CYCLE_MS    5U            /* tWR max */
#define EEPROM_I2C_TIMEOUT_MS    10U

/* ------------------------------------------------------------------ */
/*  Private Variables                                                    */
/* ------------------------------------------------------------------ */
static volatile uint32_t nvram_error_count;
static SemaphoreHandle_t nvram_completion_sem;  /* For async read signalling */

extern I2C_HandleTypeDef hi2c1;

/* ------------------------------------------------------------------ */
/*  I2C Hardware Abstraction                                            */
/* ------------------------------------------------------------------ */

static HAL_StatusTypeDef eeprom_i2c_write(uint32_t mem_addr, const uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Write(&hi2c1,
                             EEPROM_DEVICE_ADDR,
                             mem_addr,
                             I2C_MEMADD_SIZE_16BIT,
                             (uint8_t *)data,
                             len,
                             EEPROM_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef eeprom_i2c_read(uint32_t mem_addr, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Read(&hi2c1,
                            EEPROM_DEVICE_ADDR,
                            mem_addr,
                            I2C_MEMADD_SIZE_16BIT,
                            data,
                            len,
                            EEPROM_I2C_TIMEOUT_MS);
}

/* ------------------------------------------------------------------ */
/*  Page-Aware Write                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Write data across page boundaries. The AT24C512 performs
 *        internal address auto-increment within a 128-byte page,
 *        but wraps at page boundaries.
 */
static bool eeprom_page_write(uint32_t address, const uint8_t *buffer, uint16_t len)
{
    uint16_t remaining = len;
    uint16_t offset    = 0U;

    while (remaining > 0U) {
        uint16_t page_offset = (uint16_t)(address & (EEPROM_PAGE_SIZE_DEVICE - 1U));
        uint16_t chunk = EEPROM_PAGE_SIZE_DEVICE - page_offset;
        if (chunk > remaining) {
            chunk = remaining;
        }

        if (eeprom_i2c_write(address, &buffer[offset], chunk) != HAL_OK) {
            nvram_error_count++;
            return false;
        }

        /* Wait for write cycle completion */
        vTaskDelay(pdMS_TO_TICKS(EEPROM_WRITE_CYCLE_MS));

        address   += chunk;
        offset    += chunk;
        remaining -= chunk;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

void nvram_init(void)
{
    nvram_error_count = 0U;
    nvram_completion_sem = xSemaphoreCreateBinary();
}

bool nvram_read(uint32_t address, uint8_t *buffer, uint16_t len)
{
    if (buffer == NULL || len == 0U) {
        return false;
    }

    /* Lock the I2C bus */
    if (xSemaphoreTake(mutexNvramCache, pdMS_TO_TICKS(100U)) != pdPASS) {
        nvram_error_count++;
        return false;
    }

    HAL_StatusTypeDef status = eeprom_i2c_read(address, buffer, len);
    xSemaphoreGive(mutexNvramCache);

    if (status != HAL_OK) {
        nvram_error_count++;
        return false;
    }
    return true;
}

bool nvram_write(uint32_t address, const uint8_t *buffer, uint16_t len)
{
    if (buffer == NULL || len == 0U) {
        return false;
    }

    if (xSemaphoreTake(mutexNvramCache, pdMS_TO_TICKS(100U)) != pdPASS) {
        nvram_error_count++;
        return false;
    }

    bool ok = eeprom_page_write(address, buffer, len);
    xSemaphoreGive(mutexNvramCache);

    return ok;
}

BaseType_t nvram_write_async(uint32_t address, const uint8_t *buffer, uint16_t len)
{
    nvram_cmd_item_t cmd;
    cmd.cmd     = 1U;  /* write */
    cmd.address = address;
    cmd.len     = len;
    cmd.buffer  = (void *)buffer;  /* Caller must ensure buffer persistence */

    return xQueueSend(qNvramCmd, &cmd, pdMS_TO_TICKS(50U));
}

bool nvram_read_async(uint32_t address, uint8_t *buffer, uint16_t len, TickType_t timeout)
{
    nvram_cmd_item_t cmd;
    cmd.cmd     = 0U;  /* read */
    cmd.address = address;
    cmd.len     = len;
    cmd.buffer  = buffer;

    if (xQueueSend(qNvramCmd, &cmd, timeout) != pdPASS) {
        return false;
    }

    /* Wait for completion signal from NvramTask */
    if (xSemaphoreTake(nvram_completion_sem, timeout) != pdPASS) {
        return false;
    }
    return true;
}

bool nvram_erase(uint32_t address, uint16_t len)
{
    /* Fill with 0xFF in chunks */
    static uint8_t erase_buf[EEPROM_PAGE_SIZE_DEVICE];
    memset(erase_buf, 0xFF, sizeof(erase_buf));

    uint16_t remaining = len;
    uint32_t addr      = address;

    while (remaining > 0U) {
        uint16_t chunk = (remaining > sizeof(erase_buf)) ? sizeof(erase_buf) : remaining;
        if (!nvram_write(addr, erase_buf, chunk)) {
            return false;
        }
        addr      += chunk;
        remaining -= chunk;
    }
    return true;
}

void nvram_flush(void)
{
    /* Drain the NVRAM command queue */
    while (uxQueueMessagesWaiting(qNvramCmd) > 0U) {
        vTaskDelay(pdMS_TO_TICKS(EEPROM_WRITE_CYCLE_MS * 2U));
    }
}

uint32_t nvram_get_error_count(void)
{
    return nvram_error_count;
}

/* ------------------------------------------------------------------ */
/*  Task_Nvram                                                          */
/* ------------------------------------------------------------------ */
void Task_Nvram(void *pvParameters)
{
    (void)pvParameters;
    nvram_cmd_item_t cmd;

    for (;;) {
        /* Block waiting for a command */
        if (xQueueReceive(qNvramCmd, &cmd, portMAX_DELAY) == pdPASS) {
            switch (cmd.cmd) {
            case 0U: { /* read */
                bool ok = nvram_read(cmd.address, (uint8_t *)cmd.buffer, cmd.len);
                (void)ok;
                /* Signal completion to the waiting task */
                xSemaphoreGive(nvram_completion_sem);
                break;
            }
            case 1U: { /* write */
                (void)nvram_write(cmd.address, (const uint8_t *)cmd.buffer, cmd.len);
                break;
            }
            case 2U: { /* erase */
                (void)nvram_erase(cmd.address, cmd.len);
                break;
            }
            case 3U: { /* flush */
                /* Already handling synchronously */
                break;
            }
            default:
                break;
            }
        }
    }
}
