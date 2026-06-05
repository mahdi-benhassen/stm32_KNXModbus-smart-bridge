/**
 * @file    nvram.h
 * @brief   Thread-safe NVRAM/EEPROM abstraction layer over I2C.
 *
 * Writes are deferred: callers submit commands via qNvramCmd. The NvramTask
 * processes them sequentially, respecting EEPROM page boundaries and
 * write-cycle delays (5 ms typical for AT24C512).
 */

#ifndef NVRAM_H
#define NVRAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "board.h"
#include "project_config.h"
#include "task_priorities.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include <stdbool.h>
#include <stddef.h>

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise the I2C EEPROM peripheral.
 */
void nvram_init(void);

/**
 * @brief  NvramTask entry. Processes commands from qNvramCmd.
 */
void Task_Nvram(void *pvParameters);

/**
 * @brief  Read a block of data from EEPROM. Synchronous call.
 *
 * @param address   Byte offset in EEPROM
 * @param buffer    Destination buffer (caller-owned)
 * @param len       Number of bytes to read
 * @return true on success, false on I2C error.
 */
bool nvram_read(uint32_t address, uint8_t *buffer, uint16_t len);

/**
 * @brief  Write a block of data to EEPROM. Synchronous call.
 *         Blocks until write cycle completes (~5 ms).
 *
 * @param address   Byte offset in EEPROM
 * @param buffer    Source buffer
 * @param len       Number of bytes to write
 * @return true on success.
 */
bool nvram_write(uint32_t address, const uint8_t *buffer, uint16_t len);

/**
 * @brief  Queue an asynchronous write command (preferred for non-critical paths).
 *         The NvramTask will process it later. The buffer must remain valid
 *         until the write is acknowledged.
 *
 * @return pdTRUE if queued, pdFALSE if queue full.
 */
BaseType_t nvram_write_async(uint32_t address, const uint8_t *buffer, uint16_t len);

/**
 * @brief  Submit a read command to the NvramTask queue. The caller blocks
 *         on a semaphore until the read completes.
 */
bool nvram_read_async(uint32_t address, uint8_t *buffer, uint16_t len, TickType_t timeout);

/**
 * @brief  Erase a region of EEPROM (write 0xFF).
 */
bool nvram_erase(uint32_t address, uint16_t len);

/**
 * @brief  Flush any pending queued writes immediately and synchronously.
 */
void nvram_flush(void);

/**
 * @brief  Return error count for diagnostics.
 */
uint32_t nvram_get_error_count(void);

#ifdef __cplusplus
}
#endif

#endif /* NVRAM_H */
