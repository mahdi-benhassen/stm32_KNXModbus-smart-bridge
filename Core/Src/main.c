/**
 * @file    main.c
 * @brief   Application entry point and FreeRTOS kernel launcher for the
 *          STM32 KNX/Modbus Smart Bridge.
 *
 * Startup sequence:
 *   1. HAL_Init() + SystemClock_Config()
 *   2. Peripheral GPIO, UART, I2C, Timers init
 *   3. Create FreeRTOS synchronization primitives (queues, mutexes)
 *   4. Create application tasks
 *   5. vTaskStartScheduler() – never returns.
 */

#include "project_config.h"
#include "task_priorities.h"
#include "shared_types.h"
#include "board.h"
#include "knx_tpuart.h"
#include "modbus_rs485.h"
#include "nvram.h"
#include "mapping_table.h"
#include "virtual_holder.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* ---- External task entry functions (defined in their respective modules) ---- */
extern void Task_KnxRx(void *pvParameters);
extern void Task_KnxTx(void *pvParameters);
extern void Task_ModbusRx(void *pvParameters);
extern void Task_ModbusTx(void *pvParameters);
extern void Task_DataBroker(void *pvParameters);
extern void Task_LogicEngine(void *pvParameters);
extern void Task_VirtualHolder(void *pvParameters);
extern void Task_Nvram(void *pvParameters);
extern void Task_Diag(void *pvParameters);

/* ---- Global handles (defined in task_priorities.h) ---- */
TaskHandle_t  hKnxRxTask        = NULL;
TaskHandle_t  hKnxTxTask        = NULL;
TaskHandle_t  hModbusRxTask     = NULL;
TaskHandle_t  hModbusTxTask     = NULL;
TaskHandle_t  hDataBrokerTask   = NULL;
TaskHandle_t  hLogicEngineTask  = NULL;
TaskHandle_t  hVirtualHolderTask = NULL;
TaskHandle_t  hNvramTask        = NULL;
TaskHandle_t  hDiagTask         = NULL;

QueueHandle_t qKnxRx        = NULL;
QueueHandle_t qKnxTx        = NULL;
QueueHandle_t qModbusRx     = NULL;
QueueHandle_t qModbusTx     = NULL;
QueueHandle_t qBrokerIn     = NULL;
QueueHandle_t qLogicEvent   = NULL;
QueueHandle_t qNvramCmd     = NULL;

SemaphoreHandle_t mutexMappingTable    = NULL;
SemaphoreHandle_t mutexModbusRegMap    = NULL;
SemaphoreHandle_t mutexNvramCache      = NULL;
SemaphoreHandle_t mutexKnxGroupObjects = NULL;

/* ---- External references ---- */
extern UART_HandleTypeDef  huart2;  /* KNX TPUART  */
extern UART_HandleTypeDef  huart3;  /* Modbus RS485 */
extern I2C_HandleTypeDef   hi2c1;   /* EEPROM       */
extern TIM_HandleTypeDef   htim2;   /* Modbus T3.5  */
extern TIM_HandleTypeDef   htim3;   /* RS485 delay  */
extern TIM_HandleTypeDef   htim4;   /* KNX byte-gap */

/* ------------------------------------------------------------------ */
/*  System Clock Configuration                                          */
/* ------------------------------------------------------------------ */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc_init = {0};
    RCC_ClkInitTypeDef clk_init = {0};
    HAL_StatusTypeDef  hal_ret;

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc_init.HSEState       = RCC_HSE_ON;
    osc_init.PLL.PLLState   = RCC_PLL_ON;
    osc_init.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc_init.PLL.PLLM       = 8U;
    osc_init.PLL.PLLN       = 336U;
    osc_init.PLL.PLLP       = RCC_PLLP_DIV2;
    osc_init.PLL.PLLQ       = 7U;
    hal_ret = HAL_RCC_OscConfig(&osc_init);
    if (hal_ret != HAL_OK) {
        /* HSE failed — fall back to HSI */
        osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSI;
        osc_init.HSIState       = RCC_HSI_ON;
        osc_init.PLL.PLLSource  = RCC_PLLSOURCE_HSI;
        osc_init.PLL.PLLM       = 16U;
        osc_init.PLL.PLLN       = 336U;
        osc_init.PLL.PLLP       = RCC_PLLP_DIV2;
        osc_init.PLL.PLLQ       = 7U;
        (void)HAL_RCC_OscConfig(&osc_init);
    }

    clk_init.ClockType      = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    clk_init.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk_init.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk_init.APB1CLKDivider = RCC_HCLK_DIV4;
    clk_init.APB2CLKDivider = RCC_HCLK_DIV2;
    hal_ret = HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_5);
    BRIDGE_ASSERT(hal_ret == HAL_OK);

    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000U);
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
    HAL_NVIC_SetPriority(SysTick_IRQn, 15U, 0U);
}

/* ------------------------------------------------------------------ */
/*  Peripheral GPIO Initialization                                      */
/* ------------------------------------------------------------------ */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /* Heartbeat LED */
    gpio.Pin  = LED_HEARTBEAT_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_HEARTBEAT_PORT, &gpio);

    /* RS485 DE/RE */
    gpio.Pin  = RS485_DE_RE_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(RS485_DE_RE_PORT, &gpio);
    RS485_DRIVER_DISABLE();

    /* Digital inputs */
    gpio.Pin  = DI_DOOR_SENSOR_PIN | DI_PIR_SENSOR_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(DI_DOOR_SENSOR_PORT, &gpio);
}

/* ------------------------------------------------------------------ */
/*  FreeRTOS Primitives Factory                                         */
/* ------------------------------------------------------------------ */
static void CreateFreeRTOSTasks(void)
{
    BaseType_t ret = pdPASS;

    /* ---- Queues ---- */
    qKnxRx      = xQueueCreate(QUEUE_KNX_RX_LENGTH,       sizeof(knx_telegram_item_t));
    qKnxTx      = xQueueCreate(QUEUE_KNX_TX_LENGTH,       sizeof(knx_telegram_item_t));
    qModbusRx   = xQueueCreate(QUEUE_MODBUS_RX_LENGTH,    sizeof(modbus_frame_item_t));
    qModbusTx   = xQueueCreate(QUEUE_MODBUS_TX_LENGTH,    sizeof(modbus_frame_item_t));
    qBrokerIn   = xQueueCreate(QUEUE_BROKER_IN_LENGTH,    sizeof(logic_event_item_t));
    qLogicEvent = xQueueCreate(QUEUE_LOGIC_EVENT_LENGTH,  sizeof(logic_event_item_t));
    qNvramCmd   = xQueueCreate(QUEUE_NVRAM_CMD_LENGTH,    sizeof(nvram_cmd_item_t));

    /* ---- Mutexes ---- */
    mutexMappingTable    = xSemaphoreCreateMutex();
    mutexModbusRegMap    = xSemaphoreCreateMutex();
    mutexNvramCache      = xSemaphoreCreateMutex();
    mutexKnxGroupObjects = xSemaphoreCreateMutex();

    /* ---- Tasks ---- */
    ret &= xTaskCreate(Task_KnxRx,          "KNX_Rx",   STACK_KNX_RX,          NULL, TSK_PRIO_KNX_RX,         &hKnxRxTask);
    ret &= xTaskCreate(Task_KnxTx,          "KNX_Tx",   STACK_KNX_TX,          NULL, TSK_PRIO_KNX_TX,         &hKnxTxTask);
    ret &= xTaskCreate(Task_ModbusRx,       "MB_Rx",    STACK_MODBUS_RX,       NULL, TSK_PRIO_MODBUS_RX,      &hModbusRxTask);
    ret &= xTaskCreate(Task_ModbusTx,       "MB_Tx",    STACK_MODBUS_TX,       NULL, TSK_PRIO_MODBUS_TX,      &hModbusTxTask);
    ret &= xTaskCreate(Task_DataBroker,     "Broker",   STACK_DATA_BROKER,     NULL, TSK_PRIO_DATA_BROKER,    &hDataBrokerTask);
    ret &= xTaskCreate(Task_LogicEngine,    "Logic",    STACK_LOGIC_ENGINE,    NULL, TSK_PRIO_LOGIC_ENGINE,   &hLogicEngineTask);
    ret &= xTaskCreate(Task_VirtualHolder,  "VHolder",  STACK_VIRTUAL_HOLDER,  NULL, TSK_PRIO_VIRTUAL_HOLDER, &hVirtualHolderTask);
    ret &= xTaskCreate(Task_Nvram,          "NVRAM",    STACK_NVRAM,           NULL, TSK_PRIO_NVRAM,          &hNvramTask);
    ret &= xTaskCreate(Task_Diag,           "Diag",     STACK_DIAG,            NULL, TSK_PRIO_DIAG,           &hDiagTask);

    BRIDGE_ASSERT(ret == pdPASS);
}

/* ------------------------------------------------------------------ */
/*  main()                                                              */
/* ------------------------------------------------------------------ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    /* ---- Hardware peripheral initialisation ---- */
    GPIO_Init();

    /* CubeMX-generated peripheral init:
     *   MX_USART2_UART_Init();  // KNX TPUART
     *   MX_USART3_UART_Init();  // Modbus RS485
     *   MX_I2C1_Init();         // EEPROM
     *   MX_TIM2_Init();         // Modbus T3.5 timer
     *   MX_TIM3_Init();         // RS485 delay timer
     *   MX_TIM4_Init();         // KNX byte-gap timer
     * These must be called before any driver init. */

    /* ---- Software module initialisation ---- */
    nvram_init();
    mapping_table_init();

    /* ---- Create FreeRTOS objects BEFORE driver init
     *      (queues/mutexes/task handles needed by driver inits) ---- */
    CreateFreeRTOSTasks();

    /* ---- Driver initialisation (requires valid task handles and queues) ---- */
    knx_tpuart_init();
    modbus_rs485_init();

    /* ---- Start the kernel ---- */
    vTaskStartScheduler();

    for (;;) {
        /* Trap: kernel start failed */
    }
}

/* ------------------------------------------------------------------ */
/*  Assertion Handler (forward-declared in project_config.h)            */
/* ------------------------------------------------------------------ */
void bridge_assert_failed(const char *file, int line)
{
    (void)file;
    (void)line;
    taskDISABLE_INTERRUPTS();
    /* Log file/line over debug UART, then blink LED in infinite loop */
    for (;;) {
        LED_HEARTBEAT_TOGGLE();
        for (volatile uint32_t i = 0U; i < 5000000U; i++) { __NOP(); }
    }
}

/* ------------------------------------------------------------------ */
/*  FreeRTOS Hook: Stack overflow detection                              */
/* ------------------------------------------------------------------ */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;) {
        LED_HEARTBEAT_TOGGLE();
        for (volatile uint32_t i = 0U; i < 1000000U; i++) { __NOP(); }
    }
}
