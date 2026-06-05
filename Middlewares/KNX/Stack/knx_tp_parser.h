/**
 * @file    knx_tp_parser.h
 * @brief   KNX Twisted Pair telegram parser and frame builder.
 */

#ifndef KNX_TP_PARSER_H
#define KNX_TP_PARSER_H

#include "knx_telegram.h"
#include <stdbool.h>
#include <stdint.h>

bool knx_tp_parse(const uint8_t *buffer, uint8_t len, knx_telegram_t *out);

uint8_t knx_tp_build(const knx_telegram_t *tel, uint8_t *buffer, uint8_t max_len);

bool knx_tp_validate_checksum(const uint8_t *buffer, uint8_t len);

#endif
