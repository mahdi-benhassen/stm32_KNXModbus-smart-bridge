/*
 * CI Stub: FreeRTOS.h — minimal type definitions for compile-check only.
 * Not used at runtime; the real FreeRTOS source is linked by the project Makefile.
 */
#ifndef FREERTOS_H
#define FREERTOS_H

#include <stdint.h>
#include <stddef.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#define pdFALSE  ((BaseType_t)0)
#define pdTRUE   ((BaseType_t)1)
#define pdPASS   (pdTRUE)
#define pdFAIL   (pdFALSE)

typedef void *      TaskHandle_t;
typedef void *      QueueHandle_t;
typedef void *      SemaphoreHandle_t;
typedef void *      TimerHandle_t;
typedef uint32_t    TickType_t;
typedef int32_t     BaseType_t;

#define portMAX_DELAY       ((TickType_t)0xFFFFFFFFUL)
#define portTICK_PERIOD_MS  ((TickType_t)1U)
#define pdMS_TO_TICKS(ms)   ((TickType_t)(ms))

#define tskIDLE_PRIORITY    ((BaseType_t)0)

/* Minimal FreeRTOS priority constants (actual values depend on configMAX_PRIORITIES) */
#define configMAX_PRIORITIES    16

/* Inline stubs for compile-check — do NOT link */
static inline void taskDISABLE_INTERRUPTS(void) { }
static inline void taskENABLE_INTERRUPTS(void)  { }
static inline void taskYIELD(void)               { }
static inline void taskENTER_CRITICAL(void)      { }
static inline void taskEXIT_CRITICAL(void)       { }

static inline TickType_t xTaskGetTickCount(void) { return 0U; }
static inline TaskHandle_t xTaskGetCurrentTaskHandle(void) { return NULL; }

static inline BaseType_t xTaskCreate(void *a, const char *b, uint32_t c, void *d, BaseType_t e, TaskHandle_t *f)
{ (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;return pdPASS; }

static inline void vTaskDelay(TickType_t ticks)                  { (void)ticks; }
static inline void vTaskDelayUntil(TickType_t *p, TickType_t t) { (void)p;(void)t; }
static inline void vTaskStartScheduler(void)                     { }

static inline BaseType_t xQueueSend(QueueHandle_t q, const void *p, TickType_t t)
{ (void)q;(void)p;(void)t;return pdPASS; }

static inline BaseType_t xQueueReceive(QueueHandle_t q, void *p, TickType_t t)
{ (void)q;(void)p;(void)t;return pdPASS; }

static inline BaseType_t xQueueSendFromISR(QueueHandle_t q, const void *p, BaseType_t *w)
{ (void)q;(void)p;(void)w;return pdPASS; }

static inline QueueHandle_t xQueueCreate(uint32_t n, uint32_t s)
{ (void)n;(void)s;return (QueueHandle_t)1; }

static inline uint32_t uxQueueMessagesWaiting(QueueHandle_t q) { (void)q;return 0U; }

static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) { return (SemaphoreHandle_t)1; }
static inline SemaphoreHandle_t xSemaphoreCreateBinary(void) { return (SemaphoreHandle_t)1; }

static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t s, TickType_t t)
{ (void)s;(void)t;return pdPASS; }

static inline BaseType_t xSemaphoreGive(SemaphoreHandle_t s)
{ (void)s;return pdPASS; }

static inline BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t s, BaseType_t *w)
{ (void)s;(void)w;return pdPASS; }

static inline void vTaskNotifyGiveFromISR(TaskHandle_t t, BaseType_t *w) { (void)t;(void)w; }
static inline uint32_t ulTaskNotifyTake(BaseType_t clear, TickType_t timeout)
{ (void)clear;(void)timeout;return 0U; }

static inline uint32_t uxTaskGetStackHighWaterMark(TaskHandle_t t) { (void)t;return 1024U; }

#define portYIELD_FROM_ISR(x)  ((void)(x))

#endif /* FREERTOS_H */
