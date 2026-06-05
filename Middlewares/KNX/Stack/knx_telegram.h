/**
 * @file    knx_telegram.h
 * @brief   KNX telegram structure definitions per TP1 specification.
 */

#ifndef KNX_TELEGRAM_H
#define KNX_TELEGRAM_H

#include <stdint.h>

#define KNX_CTRL_REPEAT_MASK     0x20U
#define KNX_CTRL_PRIORITY_MASK   0x0CU
#define KNX_CTRL_FRAME_FORMAT    0x80U  /* 0 = standard, 1 = extended */

#define KNX_PRIORITY_SYSTEM      0x00U
#define KNX_PRIORITY_ALARM       0x01U
#define KNX_PRIORITY_HIGH        0x02U
#define KNX_PRIORITY_LOW         0x03U

typedef enum {
    KNX_TPCI_UNNUMBERED_DATA  = 0x00U,
    KNX_TPCI_NUMBERED_DATA    = 0x40U,
    KNX_TPCI_CONNECT          = 0x80U,
    KNX_TPCI_DISCONNECT       = 0x81U,
    KNX_TPCI_ACK              = 0xC0U,
    KNX_TPCI_NACK             = 0xC1U
} knx_tpci_t;

typedef struct {
    uint8_t  ctrl;
    uint16_t source_addr;
    uint16_t dest_addr;
    uint8_t  npci_tpci_apci;
    uint8_t  data[15];
    uint8_t  data_len;
    uint8_t  checksum;
} knx_telegram_t;

#endif
