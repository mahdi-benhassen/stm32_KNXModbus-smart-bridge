/**
 * @file    knx_tpuart.c
 * @brief   KNX TPUART driver implementation - interrupt-driven with FreeRTOS integration.
 *
 * Timer:  TIM4 (dedicated to KNX, no conflict with Modbus TIM2).
 * Clock:  TIM4 is on APB1 at 42 MHz. Prescaler = 42-1 => 1 MHz base (1 µs/tick).
 *         Byte gap timeout = 2500 µs for KNX inter-frame pause detection.
 *
 * The ISR only resets the byte-gap timer on each received byte. The timer IRQ
 * signals end-of-frame and wakes the RX task via task notification.
 */

#include "knx_tpuart.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Private Constants                                                   */
/* ------------------------------------------------------------------ */
#define TPUART_RX_BUF_SIZE        64U
#define KNX_TIMER_PRESCALER       41U    /* 42 MHz / (41+1) = 1 MHz */
#define KNX_BYTE_GAP_US           2500U  /* 2.5 ms per KNX TP spec */

/* ------------------------------------------------------------------ */
/*  Private Variables                                                    */
/* ------------------------------------------------------------------ */
static volatile uint8_t  rx_ring[TPUART_RX_BUF_SIZE];
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;
static volatile uint32_t rx_error_count;
static volatile knx_drv_state_t knx_state = KNX_DRV_IDLE;

static TaskHandle_t knx_rx_task_handle;  /* Set in Task_KnxRx itself */

extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef  htim4;

/* ------------------------------------------------------------------ */
/*  Private Helpers                                                     */
/* ------------------------------------------------------------------ */

static void rx_ring_reset(void)
{
    taskDISABLE_INTERRUPTS();
    rx_head = 0U;
    rx_tail = 0U;
    taskENABLE_INTERRUPTS();
}

static bool rx_ring_empty(void)
{
    return (rx_head == rx_tail);
}

static uint8_t rx_ring_pop(void)
{
    uint8_t byte = rx_ring[rx_tail];
    rx_tail = (rx_tail + 1U) & (TPUART_RX_BUF_SIZE - 1U);
    return byte;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

void knx_tpuart_init(void)
{
    rx_ring_reset();
    rx_error_count = 0U;
    knx_state      = KNX_DRV_IDLE;

    /* Enable UART RXNE interrupt */
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);

    /* Configure TIM4 for byte-gap detection: 1 MHz base, one-shot mode.
     * ARR = byte_gap_us - 1 (timer counts from 0). */
    htim4.Instance->PSC   = KNX_TIMER_PRESCALER;
    htim4.Instance->ARR   = KNX_BYTE_GAP_US - 1U;
    __HAL_TIM_SET_COUNTER(&htim4, 0U);
    __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_UPDATE);
    HAL_TIM_Base_Start_IT(&htim4);
}

/* ------------------------------------------------------------------ */
/*  ISR Handlers                                                        */
/* ------------------------------------------------------------------ */

void knx_tpuart_irq_handler(UART_HandleTypeDef *huart)
{
    if (huart->Instance != KNX_UART) {
        return;
    }

    uint32_t sr = huart->Instance->SR;

    /* Overrun error — clear flag, count error, continue */
    if ((sr & UART_FLAG_ORE) != 0U) {
        rx_error_count++;
        __HAL_UART_CLEAR_OREFLAG(huart);
    }

    /* Framing error — discard noisy byte */
    if ((sr & UART_FLAG_FE) != 0U) {
        rx_error_count++;
        (void)huart->Instance->DR;
        return;
    }

    /* Noise error */
    if ((sr & UART_FLAG_NE) != 0U) {
        rx_error_count++;
        return;
    }

    /* RXNE: byte received */
    if ((sr & UART_FLAG_RXNE) != 0U) {
        uint8_t byte = (uint8_t)(huart->Instance->DR & 0xFFU);

        uint16_t next = (rx_head + 1U) & (TPUART_RX_BUF_SIZE - 1U);
        if (next != rx_tail) {
            rx_ring[rx_head] = byte;
            rx_head = next;
        } else {
            rx_error_count++;
        }

        /* Restart byte-gap timer on every received byte */
        __HAL_TIM_SET_COUNTER(&htim4, 0U);
        __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);

        knx_state = KNX_DRV_RX_ACTIVE;
    }
}

void knx_timer_irq_handler(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM_KNX_TIMEOUT) {
        return;
    }

    /* Byte-gap elapsed: telegram complete. Wake the RX task. */
    if (knx_state == KNX_DRV_RX_ACTIVE) {
        knx_state = KNX_DRV_IDLE;

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(knx_rx_task_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
}

/* ------------------------------------------------------------------ */
/*  KNX_Rx Task                                                         */
/* ------------------------------------------------------------------ */
void Task_KnxRx(void *pvParameters)
{
    (void)pvParameters;

    /* Capture own task handle for ISR notification */
    knx_rx_task_handle = xTaskGetCurrentTaskHandle();

    knx_telegram_item_t telegram;
    uint8_t  local_buf[KNX_MAX_TELEGRAM_LEN];
    uint16_t idx = 0U;

    for (;;) {
        /* Block until timer IRQ signals end-of-frame. The 100 ms timeout
         * is a safety net; the timer fires reliably at 2.5 ms byte-gap. */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100U));

        /* Drain the ring buffer */
        while (!rx_ring_empty()) {
            uint8_t byte = rx_ring_pop();

            if (idx < KNX_MAX_TELEGRAM_LEN) {
                local_buf[idx++] = byte;
            } else {
                idx = 0U;
                rx_error_count++;
                break;
            }
        }

        /* Dispatch completed telegram */
        if (idx >= 2U && knx_state == KNX_DRV_IDLE) {
            (void)memcpy(telegram.data, local_buf, idx);
            telegram.len   = (uint8_t)idx;
            telegram.flags = 0U;

            if (xQueueSend(qKnxRx, &telegram, 0) != pdPASS) {
                rx_error_count++;
            }
            idx = 0U;
        } else if (knx_state != KNX_DRV_IDLE && idx >= KNX_MAX_TELEGRAM_LEN) {
            /* Partial frame overflowed buffer before gap was detected */
            idx = 0U;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  KNX_Tx Task                                                         */
/* ------------------------------------------------------------------ */
void Task_KnxTx(void *pvParameters)
{
    (void)pvParameters;
    knx_telegram_item_t telegram;

    for (;;) {
        if (xQueueReceive(qKnxTx, &telegram, portMAX_DELAY) == pdPASS) {
            /* Ensure previous frame pause (2.5 ms — vTaskDelay is safe here,
             * outside any critical section). */
            vTaskDelay(pdMS_TO_TICKS(3U));

            /* Transmit serialisation: only one TX task exists, and it's the
             * sole writer to KNX_UART->DR. No mutex needed. */
            for (uint16_t i = 0U; i < telegram.len; i++) {
                while ((KNX_UART->SR & UART_FLAG_TXE) == 0U) {
                    /* Spin yield — worst case ~1.15 ms per char at 9600 bps */
                    taskYIELD();
                }
                KNX_UART->DR = telegram.data[i];
            }

            /* Wait for last byte to finish shifting */
            while ((KNX_UART->SR & UART_FLAG_TC) == 0U) {
                taskYIELD();
            }

            /* Post-frame pause */
            vTaskDelay(pdMS_TO_TICKS(1U));
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Diagnostics                                                         */
/* ------------------------------------------------------------------ */
uint32_t knx_tpuart_get_error_count(void)
{
    return rx_error_count;
}
