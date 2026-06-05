/**
 * @file    data_broker.c
 * @brief   Data Broker implementation — bidirectional KNX <-> Modbus translation.
 */

#include "data_broker.h"
#include "mapping_table.h"
#include <string.h>
#include <math.h>

/* ------------------------------------------------------------------ */
/*  KNX DPT Conversion (simplified subset)                              */
/* ------------------------------------------------------------------ */

float knx_dpt_to_float(const uint8_t *apdu, uint8_t dpt_id, uint8_t apdu_len)
{
    if (apdu == NULL || apdu_len == 0U) {
        return 0.0f;
    }

    switch (dpt_id) {
    case DPT_SWITCH:   /* 1.001 — 1 bit */
    case DPT_BOOL:     /* 1.002 — 1 bit */
        return ((apdu[0] & 0x01U) != 0U) ? 1.0f : 0.0f;

    case DPT_SCALING:  /* 5.001 — 8-bit unsigned */
        return (float)apdu[0];

    case DPT_PERCENT_U8: /* 5.004 — 8-bit unsigned 0..100% */
        return (float)apdu[0];

    case DPT_TEMPERATURE: /* 9.001 — 16-bit float (value * 0.01°C) */
        {
            int16_t raw = (int16_t)(((uint16_t)apdu[0] << 8U) | (uint16_t)apdu[1]);
            return (float)raw * 0.01f;
        }

    case DPT_HUMIDITY: /* 9.007 — 16-bit float (value * 0.01%) */
        {
            uint16_t raw = ((uint16_t)apdu[0] << 8U) | (uint16_t)apdu[1];
            return (float)raw * 0.01f;
        }

    case DPT_LUX: /* 9.004 — 16-bit float */
        {
            uint16_t raw = ((uint16_t)apdu[0] << 8U) | (uint16_t)apdu[1];
            return (float)raw;
        }

    case DPT_FLOAT16: /* 16-bit FP (IEEE 754 half-precision approximation) */
        {
            int16_t raw = (int16_t)(((uint16_t)apdu[0] << 8U) | (uint16_t)apdu[1]);
            return (float)raw;
        }

    case DPT_U16: /* 7.001 — 16-bit unsigned */
        return (float)(((uint16_t)apdu[0] << 8U) | (uint16_t)apdu[1]);

    case DPT_U32: /* 12.001 — 32-bit unsigned */
        return (float)(((uint32_t)apdu[0] << 24U) | ((uint32_t)apdu[1] << 16U)
                     | ((uint32_t)apdu[2] << 8U)  | (uint32_t)apdu[3]);

    default:
        return 0.0f;
    }
}

uint8_t float_to_knx_dpt(float value, uint8_t dpt_id, uint8_t *apdu_out)
{
    if (apdu_out == NULL) {
        return 0U;
    }

    switch (dpt_id) {
    case DPT_SWITCH:
    case DPT_BOOL:
        apdu_out[0] = (value != 0.0f) ? 0x01U : 0x00U;
        return 1U;

    case DPT_SCALING:
    case DPT_PERCENT_U8:
        apdu_out[0] = (uint8_t)(value < 0.0f ? 0U : (value > 255.0f ? 255U : (uint8_t)value));
        return 1U;

    case DPT_TEMPERATURE:
    case DPT_HUMIDITY:
        {
            int16_t raw = (int16_t)(value * 100.0f);
            apdu_out[0] = (uint8_t)((raw >> 8) & 0xFF);
            apdu_out[1] = (uint8_t)(raw & 0xFF);
            return 2U;
        }

    case DPT_LUX:
    case DPT_FLOAT16:
    case DPT_U16:
        {
            uint16_t raw = (uint16_t)(value < 0.0f ? 0U : (value > 65535.0f ? 65535U : (uint16_t)value));
            apdu_out[0] = (uint8_t)((raw >> 8) & 0xFF);
            apdu_out[1] = (uint8_t)(raw & 0xFF);
            return 2U;
        }

    case DPT_U32:
        {
            uint32_t raw = (uint32_t)value;
            apdu_out[0] = (uint8_t)((raw >> 24) & 0xFF);
            apdu_out[1] = (uint8_t)((raw >> 16) & 0xFF);
            apdu_out[2] = (uint8_t)((raw >> 8)  & 0xFF);
            apdu_out[3] = (uint8_t)(raw & 0xFF);
            return 4U;
        }

    default:
        apdu_out[0] = 0U;
        return 1U;
    }
}

/* ------------------------------------------------------------------ */
/*  Modbus Register <-> Float Conversion                                 */
/* ------------------------------------------------------------------ */

float modbus_reg_to_float(const uint8_t *reg_data, mb_reg_type_t reg_type)
{
    if (reg_data == NULL) {
        return 0.0f;
    }

    switch (reg_type) {
    case MB_COIL:
    case MB_DISCRETE_INPUT:
        return ((reg_data[0] & 0x01U) != 0U) ? 1.0f : 0.0f;

    case MB_HOLDING_REGISTER:
    case MB_INPUT_REGISTER:
        return (float)(((uint16_t)reg_data[0] << 8U) | (uint16_t)reg_data[1]);

    default:
        return 0.0f;
    }
}

void float_to_modbus_reg(float value, mb_reg_type_t reg_type, uint8_t *reg_out)
{
    if (reg_out == NULL) {
        return;
    }

    switch (reg_type) {
    case MB_COIL:
    case MB_DISCRETE_INPUT:
        reg_out[0] = (value != 0.0f) ? 0x01U : 0x00U;
        break;

    case MB_HOLDING_REGISTER:
    case MB_INPUT_REGISTER:
        {
            uint16_t raw = (uint16_t)(value < 0.0f ? 0U : (value > 65535.0f ? 65535U : (uint16_t)value));
            reg_out[0] = (uint8_t)((raw >> 8) & 0xFF);
            reg_out[1] = (uint8_t)(raw & 0xFF);
        }
        break;

    default:
        reg_out[0] = 0U;
        reg_out[1] = 0U;
        break;
    }
}

/* ------------------------------------------------------------------ */
/*  Scaling & Clamping                                                  */
/* ------------------------------------------------------------------ */

static float apply_channel_transform(float raw_value, const channel_config_t *cfg)
{
    if (cfg == NULL) {
        return raw_value;
    }

    float val = raw_value * cfg->scale_factor + cfg->offset;

    /* Clamp */
    if (val < cfg->value_min) {
        val = cfg->value_min;
    }
    if (val > cfg->value_max) {
        val = cfg->value_max;
    }
    return val;
}

/* ------------------------------------------------------------------ */
/*  Alarm Evaluation                                                     */
/* ------------------------------------------------------------------ */

static void evaluate_alarm(uint16_t channel_id, float value)
{
    const channel_config_t *cfg = mapping_get_channel(channel_id);
    if (cfg == NULL || cfg->alarm_enabled == 0U) {
        return;
    }

    alarm_severity_t alarm = ALARM_NONE;

    if (value > cfg->alarm_high) {
        alarm = (alarm_severity_t)cfg->alarm_severity;
    } else if (value < cfg->alarm_low) {
        alarm = (alarm_severity_t)cfg->alarm_severity;
    }

    if (alarm != ALARM_NONE) {
        logic_event_item_t event;
        event.channel_id = channel_id;
        event.source     = 0U;  /* broker */
        event.event_type = 1U;  /* alarm */
        (void)xQueueSend(qLogicEvent, &event, 0);
    }
}

/* ------------------------------------------------------------------ */
/*  DataBrokerTask                                                      */
/* ------------------------------------------------------------------ */
void Task_DataBroker(void *pvParameters)
{
    (void)pvParameters;

    knx_telegram_item_t  knx_tel;
    modbus_frame_item_t  mb_frame;
    logic_event_item_t   broker_event;

    for (;;) {
        /* Check KNX inbound queue first (higher priority source) */
        while (xQueueReceive(qKnxRx, &knx_tel, 0) == pdPASS) {
            /* Decode basic telegram structure:
             * [0] = control byte
             * [1..2] = source address
             * [3..4] = destination address (group or individual)
             * [5] = length (APDU length)
             * [6..6+n] = APDU
             * After APDU: checksum
             */
            if (knx_tel.len < 6U) {
                continue;
            }

            uint8_t ctrl_byte = knx_tel.data[0];
            bool    is_group  = ((ctrl_byte & 0x80U) == 0U);

            if (!is_group) {
                continue;  /* Only process group telegrams */
            }

            uint16_t dest_addr = ((uint16_t)knx_tel.data[3] << 8U) | (uint16_t)knx_tel.data[4];
            uint8_t  apdu_len  = knx_tel.data[5] & 0x0FU;
            uint8_t *apdu_data = &knx_tel.data[6];

            /* Look up mapping */
            if (xSemaphoreTake(mutexMappingTable, pdMS_TO_TICKS(10U)) != pdPASS) {
                continue;
            }

            uint16_t ch_id = mapping_find_by_knx_addr(dest_addr);
            if (ch_id == 0xFFFFU) {
                xSemaphoreGive(mutexMappingTable);
                continue;  /* No mapping for this group address */
            }

            const channel_config_t *cfg = mapping_get_channel(ch_id);
            if (cfg == NULL || cfg->active == 0U) {
                xSemaphoreGive(mutexMappingTable);
                continue;
            }

            /* Check flow direction */
            if (cfg->flow_direction == FLOW_MODBUS_TO_KNX) {
                xSemaphoreGive(mutexMappingTable);
                continue;  /* One-way Modbus -> KNX, ignore KNX write */
            }

            /* Copy channel config fields before releasing mutex
             * (other tasks may modify the mapping table). */
            uint8_t      mb_type_local    = cfg->mb_reg_type;
            uint16_t     mb_addr_local    = cfg->mb_reg_addr;
            uint8_t      knx_dpt_local    = cfg->knx_dpt;
            float        scale_local      = cfg->scale_factor;
            float        offset_local     = cfg->offset;
            float        min_local        = cfg->value_min;
            float        max_local        = cfg->value_max;

            xSemaphoreGive(mutexMappingTable);

            /* Convert KNX DPT to float */
            float raw_value  = knx_dpt_to_float(apdu_data, knx_dpt_local, apdu_len);
            float scaled_val = raw_value * scale_local + offset_local;
            if (scaled_val < min_local) { scaled_val = min_local; }
            if (scaled_val > max_local) { scaled_val = max_local; }

            /* Translate to Modbus and send */
            modbus_frame_item_t out_frame;
            (void)memset(&out_frame, 0, sizeof(out_frame));

            /* Construct Modbus write single register request (FC 06) */
            out_frame.data[0] = MODBUS_SLAVE_ADDRESS;
            out_frame.data[1] = 0x06U;  /* Write Single Register */
            out_frame.data[2] = (uint8_t)((mb_addr_local >> 8) & 0xFF);
            out_frame.data[3] = (uint8_t)(mb_addr_local & 0xFF);

            float_to_modbus_reg(scaled_val, (mb_reg_type_t)mb_type_local, &out_frame.data[4]);
            out_frame.len = 6U;

            /* Compute Modbus CRC */
            uint16_t crc = 0xFFFFU;
            for (uint8_t i = 0U; i < out_frame.len; i++) {
                crc ^= out_frame.data[i];
                for (uint8_t j = 0U; j < 8U; j++) {
                    if ((crc & 0x0001U) != 0U) {
                        crc = (crc >> 1U) ^ 0xA001U;
                    } else {
                        crc = crc >> 1U;
                    }
                }
            }
            out_frame.data[out_frame.len]     = (uint8_t)(crc & 0xFF);
            out_frame.data[out_frame.len + 1U] = (uint8_t)((crc >> 8) & 0xFF);
            out_frame.len += 2U;

            (void)xQueueSend(qModbusTx, &out_frame, pdMS_TO_TICKS(10U));

            /* Evaluate alarm */
            evaluate_alarm(ch_id, scaled_val);

            /* Post event to logic engine */
            broker_event.channel_id = ch_id;
            broker_event.source     = 0U;  /* KNX source */
            broker_event.event_type = 0U;  /* value change */
            (void)xQueueSend(qLogicEvent, &broker_event, 0);
        }

        /* Check Modbus inbound queue */
        while (xQueueReceive(qModbusRx, &mb_frame, 0) == pdPASS) {
            if (mb_frame.len < 4U) {
                continue;
            }

            uint8_t func_code = mb_frame.data[1];
            uint16_t reg_addr = ((uint16_t)mb_frame.data[2] << 8U) | (uint16_t)mb_frame.data[3];
            mb_reg_type_t reg_type;

            switch (func_code) {
            case 0x01U: case 0x05U: case 0x0FU:
                reg_type = MB_COIL;
                break;
            case 0x03U: case 0x06U: case 0x10U:
                reg_type = MB_HOLDING_REGISTER;
                break;
            case 0x04U:
                reg_type = MB_INPUT_REGISTER;
                break;
            default:
                continue;
            }

            if (xSemaphoreTake(mutexMappingTable, pdMS_TO_TICKS(10U)) != pdPASS) {
                continue;
            }

            uint16_t ch_id = mapping_find_by_modbus_reg(reg_type, reg_addr);
            if (ch_id == 0xFFFFU) {
                xSemaphoreGive(mutexMappingTable);
                continue;
            }

            const channel_config_t *cfg = mapping_get_channel(ch_id);
            if (cfg == NULL || cfg->active == 0U) {
                xSemaphoreGive(mutexMappingTable);
                continue;
            }

            if (cfg->flow_direction == FLOW_KNX_TO_MODBUS) {
                xSemaphoreGive(mutexMappingTable);
                continue;
            }

            /* Extract value from Modbus frame */
            float raw_value = 0.0f;
            if (mb_frame.len >= 6U) {
                raw_value = modbus_reg_to_float(&mb_frame.data[4], reg_type);
            } else if (func_code == 0x05U && mb_frame.len >= 5U) {
                raw_value = (mb_frame.data[4] == 0xFFU) ? 1.0f : 0.0f;
            }

            /* Copy channel fields before releasing mutex */
            uint8_t  knx_dpt_l = cfg->knx_dpt;
            uint16_t knx_ga_l  = cfg->knx_group_addr;
            float    scale_l    = cfg->scale_factor;
            float    offset_l   = cfg->offset;
            float    min_l      = cfg->value_min;
            float    max_l      = cfg->value_max;

            xSemaphoreGive(mutexMappingTable);

            float scaled_val = raw_value * scale_l + offset_l;
            if (scaled_val < min_l)  { scaled_val = min_l; }
            if (scaled_val > max_l)  { scaled_val = max_l; }

            /* Translate to KNX */
            knx_telegram_item_t knx_out;
            (void)memset(&knx_out, 0, sizeof(knx_out));

            /* Build minimal KNX group telegram */
            knx_out.data[0] = 0xBCU;  /* Standard group write, no repeat */
            /* Source address (our device) */
            knx_out.data[1] = (uint8_t)((KNX_INDIVIDUAL_ADDRESS >> 8) & 0xFF);
            knx_out.data[2] = (uint8_t)(KNX_INDIVIDUAL_ADDRESS & 0xFF);
            /* Destination */
            knx_out.data[3] = (uint8_t)((knx_ga_l >> 8) & 0xFF);
            knx_out.data[4] = (uint8_t)(knx_ga_l & 0xFF);

            uint8_t apdu[8];
            uint8_t apdu_len = float_to_knx_dpt(scaled_val, knx_dpt_l, apdu);

            knx_out.data[5] = (uint8_t)(apdu_len & 0x0FU);
            (void)memcpy(&knx_out.data[6], apdu, apdu_len);
            knx_out.len = 6U + apdu_len;

            /* Append KNX checksum (simple XOR of all bytes) */
            uint8_t checksum = 0xFFU;
            for (uint8_t i = 0U; i < knx_out.len; i++) {
                checksum ^= knx_out.data[i];
            }
            knx_out.data[knx_out.len] = checksum;
            knx_out.len++;

            (void)xQueueSend(qKnxTx, &knx_out, pdMS_TO_TICKS(10U));

            /* Evaluate alarm */
            evaluate_alarm(ch_id, scaled_val);

            /* Post event to logic engine */
            broker_event.channel_id = ch_id;
            broker_event.source     = 1U;  /* Modbus source */
            broker_event.event_type = 0U;
            (void)xQueueSend(qLogicEvent, &broker_event, 0);
        }

        /* Yield to allow lower priority tasks to run */
        vTaskDelay(pdMS_TO_TICKS(5U));
    }
}
