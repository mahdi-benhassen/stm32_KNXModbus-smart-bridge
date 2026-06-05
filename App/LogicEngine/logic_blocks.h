/**
 * @file    logic_blocks.h
 * @brief   Logic engine: evaluates up to 50 configurable logic blocks.
 *
 * Each block reads from source channels (mapping table), applies the
 * configured operator, and writes the result to a destination channel.
 *
 * Blocks are evaluated round-robin by the LogicEngineTask.
 */

#ifndef LOGIC_BLOCKS_H
#define LOGIC_BLOCKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "shared_types.h"
#include "task_priorities.h"

/* ------------------------------------------------------------------ */
/*  Public Types                                                        */
/* ------------------------------------------------------------------ */

/** Per-block runtime state (NOT persisted in EEPROM — regenerated each boot) */
typedef struct {
    float    last_output;
    uint32_t last_eval_tick;
    uint32_t eval_count;
    uint8_t  error;      /* 0 = OK, 1 = div by zero, 2 = invalid source, 3 = overflow */
} logic_runtime_t;

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise the logic engine runtime states.
 */
void logic_engine_init(void);

/**
 * @brief  Evaluate a single logic block by index (0..MAX_LOGIC_BLOCKS-1).
 *         Reads source channels from the mapping cache, executes the
 *         operation, and writes the result (or triggers an action).
 *
 * @return The computed float value, or NAN on error.
 */
float logic_evaluate_block(uint16_t block_id);

/**
 * @brief  Get the runtime state for a logic block.
 */
const logic_runtime_t *logic_get_runtime(uint16_t block_id);

/**
 * @brief  Get a human-readable error description for a block.
 */
const char *logic_get_error_string(uint16_t block_id);

/**
 * @brief  LogicEngineTask entry point.
 */
void Task_LogicEngine(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* LOGIC_BLOCKS_H */
