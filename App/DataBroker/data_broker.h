/**
 * @file    data_broker.h
 * @brief   Data Broker: translates between KNX group objects and Modbus
 *          registers using the 250-channel mapping table.
 *
 * The DataBrokerTask is the heart of the bridge. It:
 *   1. Reads from qKnxRx and qModbusRx.
 *   2. Looks up the corresponding channel in the mapping table.
 *   3. Translates the data value between KNX DPT format and Modbus register format.
 *   4. Applies scaling, offset, and min/max clamping.
 *   5. Evaluates alarm thresholds.
 *   6. Dispatches translated telegrams/frames to the outbound queues.
 *   7. Posts event notifications to qLogicEvent for the Logic Engine.
 */

#ifndef DATA_BROKER_H
#define DATA_BROKER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "shared_types.h"
#include "project_config.h"
#include "task_priorities.h"
#include "FreeRTOS.h"
#include "queue.h"

/* ------------------------------------------------------------------ */
/*  Value Conversion Prototypes                                         */
/* ------------------------------------------------------------------ */

/**
 * @brief  Convert a raw KNX APDU payload to a float32 data_value_t
 *         according to the given DPT.
 */
float knx_dpt_to_float(const uint8_t *apdu, uint8_t dpt_id, uint8_t apdu_len);

/**
 * @brief  Convert a float32 to a KNX APDU byte sequence for a given DPT.
 * @return Number of bytes written to apdu_out.
 */
uint8_t float_to_knx_dpt(float value, uint8_t dpt_id, uint8_t *apdu_out);

/**
 * @brief  Convert a Modbus register value to float32.
 */
float modbus_reg_to_float(const uint8_t *reg_data, mb_reg_type_t reg_type);

/**
 * @brief  Convert a float32 to a Modbus register value.
 */
void float_to_modbus_reg(float value, mb_reg_type_t reg_type, uint8_t *reg_out);

/* ------------------------------------------------------------------ */
/*  Data Broker Task Entry                                              */
/* ------------------------------------------------------------------ */
void Task_DataBroker(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* DATA_BROKER_H */
