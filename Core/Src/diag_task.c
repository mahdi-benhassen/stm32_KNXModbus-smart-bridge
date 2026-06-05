/**
 * @file    diag_task.c
 * @brief   Diagnostics task: heartbeat LED, watchdog kick, runtime statistics.
 *          Placed in Core/Src as it binds to low-level hardware.
 */

#include "project_config.h"
#include "task_priorities.h"
#include "board.h"
#include "shared_types.h"
#include "knx_tpuart.h"
#include "modbus_rs485.h"
#include "nvram.h"
#include "virtual_holder.h"
#include "FreeRTOS.h"
#include "task.h"

/* ------------------------------------------------------------------ */
/*  Task_Diag                                                           */
/* ------------------------------------------------------------------ */
void Task_Diag(void *pvParameters)
{
    (void)pvParameters;

    TickType_t last_wake = xTaskGetTickCount();
    uint32_t   cycle_count = 0U;

    for (;;) {
        /* Toggle heartbeat LED */
        if ((cycle_count & 0x01U) != 0U) {
            LED_HEARTBEAT_ON();
        } else {
            LED_HEARTBEAT_OFF();
        }

        /* Kick independent watchdog (if enabled) */
        /* HAL_IWDG_Refresh(&hiwdg); */

        /* Gather runtime statistics (every 10 s) */
        if ((cycle_count % 10U) == 0U) {
            /* Collect stack high-water marks */
            uint32_t stack_knx_rx    = uxTaskGetStackHighWaterMark(hKnxRxTask);
            uint32_t stack_knx_tx    = uxTaskGetStackHighWaterMark(hKnxTxTask);
            uint32_t stack_mb_rx     = uxTaskGetStackHighWaterMark(hModbusRxTask);
            uint32_t stack_mb_tx     = uxTaskGetStackHighWaterMark(hModbusTxTask);
            uint32_t stack_broker    = uxTaskGetStackHighWaterMark(hDataBrokerTask);
            uint32_t stack_logic     = uxTaskGetStackHighWaterMark(hLogicEngineTask);
            uint32_t stack_vholder   = uxTaskGetStackHighWaterMark(hVirtualHolderTask);
            uint32_t stack_nvram     = uxTaskGetStackHighWaterMark(hNvramTask);
            uint32_t stack_diag      = uxTaskGetStackHighWaterMark(hDiagTask);

            /* Error counts */
            uint32_t knx_err    = knx_tpuart_get_error_count();
            uint32_t mb_err     = modbus_rs485_get_error_count();
            uint32_t nvram_err  = nvram_get_error_count();
            bool     vh_alarm   = vh_is_alarm_active();

            /* Suppress warnings about unused variables (values logged to
             * debug UART / SWO in production). */
            (void)stack_knx_rx;  (void)stack_knx_tx;
            (void)stack_mb_rx;   (void)stack_mb_tx;
            (void)stack_broker;  (void)stack_logic;
            (void)stack_vholder; (void)stack_nvram;
            (void)stack_diag;
            (void)knx_err; (void)mb_err; (void)nvram_err; (void)vh_alarm;

            /* Critical: if any task stack is below 10% free, assert */
            BRIDGE_ASSERT(stack_knx_rx    > (STACK_KNX_RX          / 10U));
            BRIDGE_ASSERT(stack_broker    > (STACK_DATA_BROKER     / 10U));
            BRIDGE_ASSERT(stack_logic     > (STACK_LOGIC_ENGINE    / 10U));
            BRIDGE_ASSERT(stack_vholder   > (STACK_VIRTUAL_HOLDER  / 10U));
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(WATCHDOG_REFRESH_MS));
        cycle_count++;
    }
}
