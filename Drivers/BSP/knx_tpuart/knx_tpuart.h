/**
 * @file    knx_tpuart.h
 * @brief   Thread-safe KNX TPUART driver wrapper (9600 bps).
 *
 * Architecture:
 *   - UART RX interrupt collects bytes into a local ring buffer.
 *   - A hardware timer (TIM2) measures the inter-byte gap; when the gap
 *     exceeds the KNX defined pause, the accumulated frame is posted to
 *     qKnxRx via xQueueSendFromISR.
 *   - TX is driven by the KNX_Tx task, which dequeues from qKnxTx and
 *     transmits byte-by-byte using the UART TXE interrupt.
 */

#ifndef KNX_TPUART_H
#define KNX_TPUART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "board.h"
#include "project_config.h"
#include "task_priorities.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

/* ------------------------------------------------------------------ */
/*  KNX Byte Timing Constants                                          */
/* ------------------------------------------------------------------ */
#define KNX_CHAR_TIMEOUT_US      ((1000000UL * 11U) / KNX_TPUART_BAUDRATE) /* ~1145 us */
#define KNX_FRAME_PAUSE_US       2500UL   /* KNX spec: 2.5 ms pause between frames */

/* ------------------------------------------------------------------ */
/*  Driver State                                                       */
/* ------------------------------------------------------------------ */
typedef enum {
    KNX_DRV_IDLE,
    KNX_DRV_RX_ACTIVE,
    KNX_DRV_TX_ACTIVE,
    KNX_DRV_ERROR
} knx_drv_state_t;

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialize the KNX TPUART hardware, UART, timer, and DMA if used.
 *         Must be called once before the scheduler starts.
 */
void knx_tpuart_init(void);

/**
 * @brief  KNX_Rx task entry. Blocks on UART notification, assembles
 *         telegrams, and posts them to qKnxRx.
 */
void Task_KnxRx(void *pvParameters);

/**
 * @brief  KNX_Tx task entry. Blocks on qKnxTx, transmits telegrams
 *         byte-by-byte via UART with proper inter-frame spacing.
 */
void Task_KnxTx(void *pvParameters);

/**
 * @brief  UART interrupt handler for the KNX USART.
 *         Called from USART2_IRQHandler in stm32f4xx_it.c.
 */
void knx_tpuart_irq_handler(UART_HandleTypeDef *huart);

/**
 * @brief  Timer interrupt handler for byte-gap timeout detection.
 */
void knx_timer_irq_handler(TIM_HandleTypeDef *htim);

/**
 * @brief  Report driver health: consecutive errors, overruns, etc.
 */
uint32_t knx_tpuart_get_error_count(void);

#ifdef __cplusplus
}
#endif

#endif /* KNX_TPUART_H */
