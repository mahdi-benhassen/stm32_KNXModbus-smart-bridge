/**
 * @file    modbus_rs485.h
 * @brief   Thread-safe RS485 Modbus RTU driver wrapper.
 *
 * Key design decisions:
 *   - T1.5 (inter-char timeout) and T3.5 (inter-frame timeout) handled by TIM2
 *     using the same hardware as KNX timeout, multiplexed via state.
 *   - DE/RE pin toggled with microsecond precision via TIM3 one-shot timer.
 *   - Half-duplex: driver is normally disabled (receive mode), enabled only
 *     during transmission.
 */

#ifndef MODBUS_RS485_H
#define MODBUS_RS485_H

#ifdef __cplusplus
extern "C" {
#endif

#include "board.h"
#include "project_config.h"
#include "task_priorities.h"
#include "FreeRTOS.h"
#include "queue.h"

/* ------------------------------------------------------------------ */
/*  Modbus timing (in timer ticks, assuming 1 µs per tick)              */
/* ------------------------------------------------------------------ */
#define MODBUS_T15_TIMEOUT_TICKS   ((uint32_t)(MODBUS_CHAR_TIMEOUT_T15_MS * 1000UL))
#define MODBUS_T35_TIMEOUT_TICKS   ((uint32_t)(MODBUS_FRAME_TIMEOUT_T35_MS * 1000UL))

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialise the RS485 UART and direction-control GPIO.
 */
void modbus_rs485_init(void);

/**
 * @brief  Modbus_Rx task entry. Assembles frames via T1.5/T3.5 timing,
 *         validates CRC, and posts to qModbusRx.
 */
void Task_ModbusRx(void *pvParameters);

/**
 * @brief  Modbus_Tx task entry. Fetches frames from qModbusTx, asserts
 *         DE=1, transmits with pre/post delays, de-asserts DE.
 */
void Task_ModbusTx(void *pvParameters);

/**
 * @brief  UART interrupt handler for the Modbus USART.
 */
void modbus_rs485_irq_handler(UART_HandleTypeDef *huart);

/**
 * @brief  Timer interrupt handler for T1.5/T3.5 and RS485 delay timing.
 */
void modbus_timer_irq_handler(TIM_HandleTypeDef *htim);

/**
 * @brief  Return frame error / overrun count for diagnostics.
 */
uint32_t modbus_rs485_get_error_count(void);

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_RS485_H */
