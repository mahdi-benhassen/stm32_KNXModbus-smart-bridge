/**
 * @file    task_priorities.h
 * @brief   FreeRTOS task allocation map: priorities, stack sizes, and queue
 *          dimensions for the KNX/Modbus Smart Bridge.
 *
 * Priority scheme (FreeRTOS: higher numeric value = higher priority):
 *
 *   Priority 9  - KNX_RxTask           Must never lose a byte on the 9600 bps UART.
 *                                       Small, fast ISR-like consumer.
 *   Priority 8  - Modbus_RxTask         RS485 framing with T1.5/T3.5 timing.
 *                                       Unbuffered reception window is 3.5 char-times.
 *   Priority 7  - KNX_TxTask            Serializes KNX telegrams to TPUART.
 *   Priority 7  - Modbus_TxTask         Serializes Modbus frames, manages DE/RE pin.
 *   Priority 6  - DataBrokerTask        Reads from inbound queues, consults mapping
 *                                       table, translates and dispatches.
 *   Priority 5  - LogicEngineTask       Evaluates 50 logic blocks, PI control, dew point.
 *                                       CPU-bound periodically; yields via vTaskDelay.
 *   Priority 4  - VirtualHolderTask     Hotel-room presence state machine.
 *                                       Polls digital inputs, runs state transitions.
 *   Priority 3  - NvramTask             Deferred NVRAM write/read servicing.
 *                                       Lowest "real-time" concern; flash page writes
 *                                       are slow and must not starve protocol stacks.
 *   Priority 2  - DiagTask              Heartbeat LED, watchdog kick, runtime stats.
 *   Priority 0  - IdleTask              FreeRTOS built-in idle hook.
 *
 * Stack sizes are expressed in words (uint32_t on Cortex-M).
 * Final values should be validated with uxTaskGetStackHighWaterMark() during
 * integration testing and tightened.
 */

#ifndef TASK_PRIORITIES_H
#define TASK_PRIORITIES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* ------------------------------------------------------------------ */
/*  Application Task Priorities                                        */
/* ------------------------------------------------------------------ */
#define TSK_PRIO_KNX_RX              (tskIDLE_PRIORITY + 9U)
#define TSK_PRIO_MODBUS_RX           (tskIDLE_PRIORITY + 8U)
#define TSK_PRIO_KNX_TX              (tskIDLE_PRIORITY + 7U)
#define TSK_PRIO_MODBUS_TX           (tskIDLE_PRIORITY + 7U)
#define TSK_PRIO_DATA_BROKER         (tskIDLE_PRIORITY + 6U)
#define TSK_PRIO_LOGIC_ENGINE        (tskIDLE_PRIORITY + 5U)
#define TSK_PRIO_VIRTUAL_HOLDER      (tskIDLE_PRIORITY + 4U)
#define TSK_PRIO_NVRAM               (tskIDLE_PRIORITY + 3U)
#define TSK_PRIO_DIAG                (tskIDLE_PRIORITY + 2U)

/* ------------------------------------------------------------------ */
/*  Task Stack Sizes (words - multiply by 4 for bytes on Cortex-M)      */
/* ------------------------------------------------------------------ */
#define STACK_KNX_RX                 512U
#define STACK_KNX_TX                 384U
#define STACK_MODBUS_RX              512U
#define STACK_MODBUS_TX              384U
#define STACK_DATA_BROKER            1536U
#define STACK_LOGIC_ENGINE           2048U
#define STACK_VIRTUAL_HOLDER         768U
#define STACK_NVRAM                  640U
#define STACK_DIAG                   256U

/* ------------------------------------------------------------------ */
/*  Queue Dimensions                                                    */
/* ------------------------------------------------------------------ */
#define QUEUE_KNX_RX_LENGTH          16U     /* Max queued inbound KNX telegrams */
#define QUEUE_KNX_TX_LENGTH          8U      /* Max queued outbound KNX telegrams */
#define QUEUE_MODBUS_RX_LENGTH       8U      /* Max queued inbound Modbus frames */
#define QUEUE_MODBUS_TX_LENGTH       8U      /* Max queued outbound Modbus frames */
#define QUEUE_BROKER_IN_LENGTH       32U     /* Unified broker input queue */
#define QUEUE_LOGIC_EVENT_LENGTH     16U     /* Event queue for logic engine triggers */
#define QUEUE_NVRAM_CMD_LENGTH       8U      /* NVRAM command queue depth */

/* ------------------------------------------------------------------ */
/*  Queue Item Type Definitions                                         */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t  data[23];           /* Raw KNX telegram (max 23 bytes) */
    uint8_t  len;
    uint8_t  flags;              /* Bit0: repeat, Bit1-7: reserved */
} knx_telegram_item_t;

typedef struct {
    uint8_t   data[256];          /* Raw Modbus RTU frame */
    uint16_t  len;
    uint8_t   slave_addr;
    uint8_t   flags;              /* Bit0: broadcast, Bit1: from_rs485 */
} modbus_frame_item_t;

typedef struct {
    uint16_t channel_id;          /* Which channel triggered the event */
    uint8_t  source;              /* 0=KNX, 1=Modbus, 2=Logic, 3=VirtualHolder */
    uint8_t  event_type;          /* 0=value-change, 1=alarm, 2=profile-change */
} logic_event_item_t;

typedef struct {
    uint8_t  cmd;                 /* 0=read, 1=write, 2=erase, 3=flush */
    uint32_t address;
    uint16_t len;
    void    *buffer;              /* Caller-owned buffer; must persist until callback */
} nvram_cmd_item_t;

/* ------------------------------------------------------------------ */
/*  Global Handle Declarations                                          */
/* ------------------------------------------------------------------ */
extern TaskHandle_t  hKnxRxTask;
extern TaskHandle_t  hKnxTxTask;
extern TaskHandle_t  hModbusRxTask;
extern TaskHandle_t  hModbusTxTask;
extern TaskHandle_t  hDataBrokerTask;
extern TaskHandle_t  hLogicEngineTask;
extern TaskHandle_t  hVirtualHolderTask;
extern TaskHandle_t  hNvramTask;
extern TaskHandle_t  hDiagTask;

extern QueueHandle_t qKnxRx;
extern QueueHandle_t qKnxTx;
extern QueueHandle_t qModbusRx;
extern QueueHandle_t qModbusTx;
extern QueueHandle_t qBrokerIn;
extern QueueHandle_t qLogicEvent;
extern QueueHandle_t qNvramCmd;

extern SemaphoreHandle_t mutexMappingTable;
extern SemaphoreHandle_t mutexModbusRegMap;
extern SemaphoreHandle_t mutexNvramCache;
extern SemaphoreHandle_t mutexKnxGroupObjects;

#ifdef __cplusplus
}
#endif

#endif /* TASK_PRIORITIES_H */
