/**
 * @file    modbus_rs485.c
 * @brief   RS485 Modbus RTU driver implementation.
 *
 * Timer:  TIM2 (dedicated to Modbus). Clock: APB1 = 42 MHz.
 *         Prescaler = 41 -> 1 MHz (1 µs/tick).
 *         T3.5 timeout @ 38400 bps = 5 ms = 5000 ticks.
 *
 * The driver maintains an internal frame-assembly state machine:
 *   IDLE -> ACTIVE (on first byte, T3.5 timer restarted each byte)
 *        -> FRAME_READY (on T3.5 expiry) -> post to qModbusRx
 */

#include "modbus_rs485.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Private Constants                                                   */
/* ------------------------------------------------------------------ */
#define RS485_RX_BUF_SIZE          512U
#define MODBUS_TIMER_PRESCALER     41U       /* 42 MHz / (41+1) = 1 MHz */
#define MODBUS_T35_TICKS           5000U     /* 5 ms @ 1 MHz */

/* ------------------------------------------------------------------ */
/*  Receiver State Machine States                                       */
/* ------------------------------------------------------------------ */
typedef enum {
    MB_RX_IDLE,
    MB_RX_ACTIVE,
    MB_RX_FRAME_READY
} mb_rx_state_t;

/* ------------------------------------------------------------------ */
/*  Private Variables                                                    */
/* ------------------------------------------------------------------ */
static volatile uint8_t  rx_buffer[RS485_RX_BUF_SIZE];
static volatile uint16_t rx_index;
static volatile mb_rx_state_t rx_state = MB_RX_IDLE;
static volatile uint32_t mb_error_count;

static TaskHandle_t modbus_rx_task_handle;

extern UART_HandleTypeDef huart3;
extern TIM_HandleTypeDef  htim2;
extern TIM_HandleTypeDef  htim3;

/* ------------------------------------------------------------------ */
/*  CRC-16 (Modbus polynomial 0xA001)                                   */
/* ------------------------------------------------------------------ */
static uint16_t modbus_crc16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    for (uint16_t i = 0U; i < len; i++) {
        crc ^= (uint16_t)buf[i];
        for (uint8_t j = 0U; j < 8U; j++) {
            if ((crc & 0x0001U) != 0U) {
                crc = (crc >> 1U) ^ 0xA001U;
            } else {
                crc = crc >> 1U;
            }
        }
    }
    return crc;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

void modbus_rs485_init(void)
{
    modbus_rx_task_handle = NULL;
    rx_state       = MB_RX_IDLE;
    rx_index       = 0U;
    mb_error_count = 0U;

    RS485_DRIVER_DISABLE();

    __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);

    /* Configure TIM2: 1 MHz, T3.5 timeout = 5000 µs */
    htim2.Instance->PSC = MODBUS_TIMER_PRESCALER;
    htim2.Instance->ARR = MODBUS_T35_TICKS - 1U;
    __HAL_TIM_SET_COUNTER(&htim2, 0U);
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(&htim2);
}

/* ------------------------------------------------------------------ */
/*  ISR Handlers                                                        */
/* ------------------------------------------------------------------ */

void modbus_rs485_irq_handler(UART_HandleTypeDef *huart)
{
    if (huart->Instance != MODBUS_UART) {
        return;
    }

    uint32_t sr = huart->Instance->SR;

    if ((sr & UART_FLAG_ORE) != 0U) {
        mb_error_count++;
        __HAL_UART_CLEAR_OREFLAG(huart);
    }

    if ((sr & UART_FLAG_FE) != 0U) {
        mb_error_count++;
        (void)huart->Instance->DR;
        return;
    }

    if ((sr & UART_FLAG_NE) != 0U) {
        mb_error_count++;
    }

    if ((sr & UART_FLAG_RXNE) != 0U) {
        uint8_t byte = (uint8_t)(huart->Instance->DR & 0xFFU);

        if (rx_state == MB_RX_IDLE) {
            rx_state = MB_RX_ACTIVE;
            rx_index = 0U;
        }

        if (rx_index < RS485_RX_BUF_SIZE) {
            rx_buffer[rx_index++] = byte;
        } else {
            rx_state = MB_RX_IDLE;
            rx_index = 0U;
            mb_error_count++;
            return;
        }

        /* Restart T3.5 timer */
        __HAL_TIM_SET_COUNTER(&htim2, 0U);
        __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
        __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
    }
}

void modbus_timer_irq_handler(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM_MODBUS_TIMEOUT) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
        __HAL_TIM_DISABLE_IT(htim, TIM_IT_UPDATE);

        /* T3.5 silence -> frame complete */
        if (rx_state == MB_RX_ACTIVE && rx_index > 0U) {
            rx_state = MB_RX_FRAME_READY;

            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveFromISR(modbus_rx_task_handle, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }

    if (htim->Instance == TIM_RS485_DELAY) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
        HAL_TIM_Base_Stop_IT(htim);
    }
}

/* ------------------------------------------------------------------ */
/*  Modbus_Rx Task                                                      */
/* ------------------------------------------------------------------ */
void Task_ModbusRx(void *pvParameters)
{
    (void)pvParameters;

    /* Capture own handle for ISR notification */
    modbus_rx_task_handle = xTaskGetCurrentTaskHandle();

    modbus_frame_item_t frame;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100U));

        if (rx_state == MB_RX_FRAME_READY && rx_index >= 4U) {
            uint16_t crc_calc = modbus_crc16((const uint8_t *)rx_buffer, rx_index - 2U);
            uint16_t crc_recv = (uint16_t)(rx_buffer[rx_index - 1U] << 8U)
                              | (uint16_t)rx_buffer[rx_index - 2U];

            if (crc_calc == crc_recv) {
                frame.len        = rx_index;
                frame.slave_addr = rx_buffer[0];
                frame.flags      = 0U;
                if ((rx_buffer[0] & 0x80U) != 0U) {
                    frame.flags |= 0x01U;
                }
                (void)memcpy(frame.data, (const void *)rx_buffer, rx_index);

                if (xQueueSend(qModbusRx, &frame, 0) != pdPASS) {
                    mb_error_count++;
                }
            } else {
                mb_error_count++;
            }

            rx_state = MB_RX_IDLE;
            rx_index = 0U;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Modbus_Tx Task                                                      */
/* ------------------------------------------------------------------ */
void Task_ModbusTx(void *pvParameters)
{
    (void)pvParameters;
    modbus_frame_item_t frame;

    for (;;) {
        if (xQueueReceive(qModbusTx, &frame, portMAX_DELAY) == pdPASS) {
            /* Assert driver enable — brief critical section for atomic pin toggle */
            taskENTER_CRITICAL();
            RS485_DRIVER_ENABLE();
            taskEXIT_CRITICAL();

            /* Pre-delay: wait for driver to stabilise */
            for (volatile uint32_t d = 0U; d < (SYSTEM_CORE_CLOCK_HZ / 1000000UL) * RS485_TX_PREDELAY_US; d++) {
                __NOP();
            }

            /* Transmit frame byte-by-byte */
            for (uint16_t i = 0U; i < frame.len; i++) {
                while ((MODBUS_UART->SR & UART_FLAG_TXE) == 0U) {
                    taskYIELD();
                }
                MODBUS_UART->DR = frame.data[i];
            }

            /* Wait for transmission complete */
            while ((MODBUS_UART->SR & UART_FLAG_TC) == 0U) {
                taskYIELD();
            }

            /* Post-delay: let last byte propagate on the bus */
            for (volatile uint32_t d = 0U; d < (SYSTEM_CORE_CLOCK_HZ / 1000000UL) * RS485_TX_POSTDELAY_US; d++) {
                __NOP();
            }

            /* Release driver back to receive mode */
            taskENTER_CRITICAL();
            RS485_DRIVER_DISABLE();
            taskEXIT_CRITICAL();
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Diagnostics                                                         */
/* ------------------------------------------------------------------ */
uint32_t modbus_rs485_get_error_count(void)
{
    return mb_error_count;
}
