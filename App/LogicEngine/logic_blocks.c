/**
 * @file    logic_blocks.c
 * @brief   Logic engine implementation: 50-block evaluator with expression
 *          parsing, PI control, and dew point algorithms.
 *
 * Block types:
 *   - Boolean: AND, OR, NOT, XOR
 *   - Comparison: GT, LT, GE, LE, EQ, NEQ
 *   - Arithmetic: ADD, SUB, MUL, DIV
 *   - Conditional: IF_THEN_ELSE
 *   - Algorithmic: PI_CONTROL, DEW_POINT (delegated to pi_controller / dew_point)
 */

#include "logic_blocks.h"
#include "mapping_table.h"
#include "data_broker.h"
#include "pi_controller.h"
#include "dew_point.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  Module Variables                                                     */
/* ------------------------------------------------------------------ */
static logic_runtime_t g_runtime[MAX_LOGIC_BLOCKS];

/* Error message lookup */
static const char *g_error_strings[] = {
    "OK",
    "Division by zero",
    "Invalid source channel",
    "Arithmetic overflow"
};

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

void logic_engine_init(void)
{
    (void)memset(g_runtime, 0, sizeof(g_runtime));
}

const logic_runtime_t *logic_get_runtime(uint16_t block_id)
{
    if (block_id >= MAX_LOGIC_BLOCKS) {
        return NULL;
    }
    return &g_runtime[block_id];
}

const char *logic_get_error_string(uint16_t block_id)
{
    if (block_id >= MAX_LOGIC_BLOCKS ||
        g_runtime[block_id].error > 3U) {
        return "Unknown error";
    }
    return g_error_strings[g_runtime[block_id].error];
}

/* ------------------------------------------------------------------ */
/*  Source Channel Value Retrieval                                      */
/* ------------------------------------------------------------------ */

/**
 * @brief Read the "current value" of a channel. In a full implementation,
 *        this would read from a live register map mirror. Here, we use a
 *        static array of last-known values (updated by the data broker).
 */
static float channel_value_get(uint16_t channel_id)
{
    if (channel_id >= MAX_DATA_CHANNELS) {
        return NAN;
    }

    /* In production, read from a global register map maintained by the
     * data broker. For this implementation, read from the mapping table
     * config and apply identity transform as a placeholder. */
    const channel_config_t *cfg = mapping_get_channel(channel_id);
    if (cfg == NULL || cfg->active == 0U) {
        return NAN;
    }

    /* Placeholder: return the last value that was written.
     * A real implementation maintains g_value_table[MAX_DATA_CHANNELS]
     * updated atomically by Task_DataBroker. */
    return 0.0f;  /* Stub — replace with g_value_table[channel_id] */
}

/**
 * @brief Write a value back to a destination channel.
 */
static void channel_value_set(uint16_t channel_id, float value)
{
    if (channel_id >= MAX_DATA_CHANNELS) {
        return;
    }

    const channel_config_t *cfg = mapping_get_channel(channel_id);
    if (cfg == NULL || cfg->active == 0U) {
        return;
    }

    /* Clamp to channel limits */
    if (value < cfg->value_min) {
        value = cfg->value_min;
    }
    if (value > cfg->value_max) {
        value = cfg->value_max;
    }

    /* In production: write to g_value_table[channel_id], then trigger
     * the data broker to push to both KNX and Modbus sides.
     * For now, we construct telegrams directly. */

    /* KNX side */
    {
        knx_telegram_item_t knx_out;
        (void)memset(&knx_out, 0, sizeof(knx_out));

        knx_out.data[0] = 0xBCU;  /* Group write */
        knx_out.data[1] = (uint8_t)((KNX_INDIVIDUAL_ADDRESS >> 8) & 0xFF);
        knx_out.data[2] = (uint8_t)(KNX_INDIVIDUAL_ADDRESS & 0xFF);
        knx_out.data[3] = (uint8_t)((cfg->knx_group_addr >> 8) & 0xFF);
        knx_out.data[4] = (uint8_t)(cfg->knx_group_addr & 0xFF);

        uint8_t  apdu_buf[8];
        uint8_t  apdu_len = float_to_knx_dpt(value, cfg->knx_dpt, apdu_buf);

        knx_out.data[5] = (uint8_t)(apdu_len & 0x0FU);
        (void)memcpy(&knx_out.data[6], apdu_buf, apdu_len);
        knx_out.len = 6U + apdu_len;

        uint8_t checksum = 0xFFU;
        for (uint8_t i = 0U; i < knx_out.len; i++) {
            checksum ^= knx_out.data[i];
        }
        knx_out.data[knx_out.len] = checksum;
        knx_out.len++;

        (void)xQueueSend(qKnxTx, &knx_out, pdMS_TO_TICKS(10U));
    }

    /* Modbus side */
    {
        modbus_frame_item_t mb_out;
        (void)memset(&mb_out, 0, sizeof(mb_out));

        mb_out.data[0] = MODBUS_SLAVE_ADDRESS;
        mb_out.data[1] = 0x06U;  /* Write Single Register */
        mb_out.data[2] = (uint8_t)((cfg->mb_reg_addr >> 8) & 0xFF);
        mb_out.data[3] = (uint8_t)(cfg->mb_reg_addr & 0xFF);
        float_to_modbus_reg(value, (mb_reg_type_t)cfg->mb_reg_type, &mb_out.data[4]);
        mb_out.len = 6U;

        /* CRC-16 */
        uint16_t crc = 0xFFFFU;
        for (uint8_t i = 0U; i < mb_out.len; i++) {
            crc ^= mb_out.data[i];
            for (uint8_t j = 0U; j < 8U; j++) {
                if ((crc & 0x0001U) != 0U) {
                    crc = (crc >> 1U) ^ 0xA001U;
                } else {
                    crc >>= 1U;
                }
            }
        }
        mb_out.data[mb_out.len]     = (uint8_t)(crc & 0xFF);
        mb_out.data[mb_out.len + 1U] = (uint8_t)((crc >> 8) & 0xFF);
        mb_out.len += 2U;

        (void)xQueueSend(qModbusTx, &mb_out, pdMS_TO_TICKS(10U));
    }
}

/* ------------------------------------------------------------------ */
/*  Block Evaluation                                                     */
/* ------------------------------------------------------------------ */

float logic_evaluate_block(uint16_t block_id)
{
    if (block_id >= MAX_LOGIC_BLOCKS) {
        return NAN;
    }

    logic_runtime_t *rt = &g_runtime[block_id];

    if (xSemaphoreTake(mutexMappingTable, pdMS_TO_TICKS(10U)) != pdPASS) {
        rt->error = 2U;
        return NAN;
    }

    const logic_block_config_t *blk = &g_mapping_cache.logic_blocks[block_id];

    if (blk->enabled == 0U) {
        xSemaphoreGive(mutexMappingTable);
        return NAN;
    }

    rt->error = 0U;

    /* Fetch operand values */
    float a = channel_value_get(blk->source_channel_a);
    float b = (blk->source_channel_b != 0xFFFFU) ? channel_value_get(blk->source_channel_b) : blk->constant_a;
    float c = (blk->source_channel_c != 0xFFFFU) ? channel_value_get(blk->source_channel_c) : blk->constant_b;

    xSemaphoreGive(mutexMappingTable);

    /* Validate operands */
    if (isnan(a) || (blk->source_channel_b != 0xFFFFU && isnan(b))
               || (blk->source_channel_c != 0xFFFFU && isnan(c))) {
        rt->error = 2U;
        return NAN;
    }

    float result = 0.0f;
    logic_op_t op = (logic_op_t)blk->op;

    switch (op) {
    case LOGIC_OP_AND:
        result = ((a != 0.0f) && (b != 0.0f)) ? 1.0f : 0.0f;
        break;

    case LOGIC_OP_OR:
        result = ((a != 0.0f) || (b != 0.0f)) ? 1.0f : 0.0f;
        break;

    case LOGIC_OP_NOT:
        result = (a == 0.0f) ? 1.0f : 0.0f;
        break;

    case LOGIC_OP_XOR:
        result = (((a != 0.0f) && (b == 0.0f)) || ((a == 0.0f) && (b != 0.0f))) ? 1.0f : 0.0f;
        break;

    case LOGIC_OP_GT:
        result = (a > b) ? 1.0f : 0.0f;
        break;

    case LOGIC_OP_LT:
        result = (a < b) ? 1.0f : 0.0f;
        break;

    case LOGIC_OP_GE:
        result = (a >= b) ? 1.0f : 0.0f;
        break;

    case LOGIC_OP_LE:
        result = (a <= b) ? 1.0f : 0.0f;
        break;

    case LOGIC_OP_EQ:
        result = (fabsf(a - b) < 1e-6f) ? 1.0f : 0.0f;
        break;

    case LOGIC_OP_NEQ:
        result = (fabsf(a - b) >= 1e-6f) ? 1.0f : 0.0f;
        break;

    case LOGIC_OP_ADD:
        result = a + b;
        break;

    case LOGIC_OP_SUB:
        result = a - b;
        break;

    case LOGIC_OP_MUL:
        result = a * b;
        break;

    case LOGIC_OP_DIV:
        if (fabsf(b) < 1e-9f) {
            rt->error = 1U;
            return NAN;
        }
        result = a / b;
        break;

    case LOGIC_OP_IF_THEN_ELSE:
        /* IF a != 0 THEN b ELSE c */
        result = (a != 0.0f) ? b : c;
        break;

    case LOGIC_OP_PI_CONTROL:
        result = pi_controller_evaluate(block_id, a, b);
        break;

    case LOGIC_OP_DEW_POINT:
        /* a = temperature (°C), b = relative humidity (%) */
        result = dew_point_calculate(a, b);
        break;

    default:
        rt->error = 3U;
        return NAN;
    }

    /* Check for overflow/non-finite */
    if (!isfinite(result)) {
        rt->error = 3U;
        return NAN;
    }

    /* Write result to destination channel if configured */
    if (blk->dest_channel != 0xFFFFU && blk->dest_channel < MAX_DATA_CHANNELS) {
        channel_value_set(blk->dest_channel, result);
    }

    rt->last_output     = result;
    rt->last_eval_tick  = xTaskGetTickCount();
    rt->eval_count++;

    return result;
}

/* ------------------------------------------------------------------ */
/*  LogicEngineTask                                                     */
/* ------------------------------------------------------------------ */
void Task_LogicEngine(void *pvParameters)
{
    (void)pvParameters;

    logic_engine_init();

    logic_event_item_t event;
    static uint16_t    round_robin_index = 0U;
    const TickType_t   eval_period       = pdMS_TO_TICKS(50U);

    for (;;) {
        /* Check for triggered events (from the data broker or virtual holder) */
        TickType_t start_tick = xTaskGetTickCount();

        while (xQueueReceive(qLogicEvent, &event, 0) == pdPASS) {
            /* Event-driven: evaluate blocks that depend on the changed channel.
             * For now, force re-evaluation of all blocks. */
            for (uint16_t i = 0U; i < MAX_LOGIC_BLOCKS; i++) {
                if (g_mapping_cache.logic_blocks[i].enabled != 0U) {
                    (void)logic_evaluate_block(i);
                }
            }
        }

        /* Periodic round-robin sweep: evaluate a subset of blocks each cycle
         * to avoid CPU starvation of lower-priority tasks. */
        uint8_t blocks_per_cycle = 5U;

        for (uint8_t round = 0U; round < blocks_per_cycle; round++) {
            if (g_mapping_cache.logic_blocks[round_robin_index].enabled != 0U) {
                (void)logic_evaluate_block(round_robin_index);
            }
            round_robin_index++;
            if (round_robin_index >= MAX_LOGIC_BLOCKS) {
                round_robin_index = 0U;
            }
        }

        /* Yield until next evaluation slot */
        TickType_t elapsed = xTaskGetTickCount() - start_tick;
        if (elapsed < eval_period) {
            vTaskDelay(eval_period - elapsed);
        } else {
            vTaskDelay(1U);  /* Minimum yield to avoid watchdog */
        }
    }
}
