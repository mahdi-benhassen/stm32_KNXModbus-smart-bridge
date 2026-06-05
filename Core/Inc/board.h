/**
 * @file    board.h
 * @brief   Board-level peripheral pin assignments and hardware abstraction
 *          macros for the STM32 KNX/Modbus Smart Bridge.
 *
 * GPIO mapping (example STM32F407VG – adjust per custom design):
 *   USART2  – KNX TPUART   (fixed 9600 bps, Rx only triggered by TX complete)
 *   USART3  – RS485 Modbus (with DE/RE pin)
 *   I2C1    – EEPROM (AT24C512 or similar)
 *   TIM4    – KNX Rx byte-gap timer (dedicated, 1 MHz)
 *   TIM2    – Modbus T3.5 frame detection timer (dedicated, 1 MHz)
 *   TIM3    – RS485 TX pre/post delay timer
 *   PE0     – RS485 DE/RE  (active-high driver enable)
 *   PB0     – Digital Input (door magnetic sensor)
 *   PB1     – Digital Input (PIR motion sensor)
 *   PC13    – Heartbeat LED
 */

#ifndef BOARD_H
#define BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* ------------------------------------------------------------------ */
/*  UART Peripheral Instance Assignments                               */
/* ------------------------------------------------------------------ */
#define KNX_UART             USART2
#define KNX_UART_IRQn        USART2_IRQn
#define KNX_UART_IRQHandler  USART2_IRQHandler
#define KNX_UART_GPIO_PORT   GPIOD
#define KNX_UART_TX_PIN      GPIO_PIN_5
#define KNX_UART_RX_PIN      GPIO_PIN_6
#define KNX_UART_AF          GPIO_AF7_USART2

#define MODBUS_UART          USART3
#define MODBUS_UART_IRQn     USART3_IRQn
#define MODBUS_UART_IRQHandler USART3_IRQHandler
#define MODBUS_UART_GPIO_PORT GPIOD
#define MODBUS_UART_TX_PIN   GPIO_PIN_8
#define MODBUS_UART_RX_PIN   GPIO_PIN_9
#define MODBUS_UART_AF       GPIO_AF7_USART3

/* RS485 Direction Control */
#define RS485_DE_RE_PORT     GPIOE
#define RS485_DE_RE_PIN      GPIO_PIN_0

#define RS485_DRIVER_ENABLE()   HAL_GPIO_WritePin(RS485_DE_RE_PORT, RS485_DE_RE_PIN, GPIO_PIN_SET)
#define RS485_DRIVER_DISABLE()  HAL_GPIO_WritePin(RS485_DE_RE_PORT, RS485_DE_RE_PIN, GPIO_PIN_RESET)

/* ------------------------------------------------------------------ */
/*  I2C EEPROM                                                         */
/* ------------------------------------------------------------------ */
#define EEPROM_I2C           I2C1
#define EEPROM_I2C_IRQn      I2C1_ER_IRQn
#define EEPROM_DEVICE_ADDR   0xA0U       /* 7-bit shifted address */

/* ------------------------------------------------------------------ */
/*  Timer Allocations                                                   */
/* ------------------------------------------------------------------ */
#define TIM_KNX_TIMEOUT      TIM4        /* KNX byte-pause timer (dedicated)  */
#define TIM_MODBUS_TIMEOUT   TIM2        /* Modbus T1.5/T3.5 frame detection  */
#define TIM_RS485_DELAY      TIM3        /* RS485 pre/post-delay one-shot     */

/* ------------------------------------------------------------------ */
/*  Digital Inputs (Virtual Holder)                                     */
/* ------------------------------------------------------------------ */
#define DI_DOOR_SENSOR_PORT  GPIOB
#define DI_DOOR_SENSOR_PIN   GPIO_PIN_0
#define DI_PIR_SENSOR_PORT   GPIOB
#define DI_PIR_SENSOR_PIN    GPIO_PIN_1

#define READ_DOOR_SENSOR()   (HAL_GPIO_ReadPin(DI_DOOR_SENSOR_PORT, DI_DOOR_SENSOR_PIN) == GPIO_PIN_SET)
#define READ_PIR_SENSOR()    (HAL_GPIO_ReadPin(DI_PIR_SENSOR_PORT, DI_PIR_SENSOR_PIN) == GPIO_PIN_SET)

/* ------------------------------------------------------------------ */
/*  Miscellaneous GPIO                                                  */
/* ------------------------------------------------------------------ */
#define LED_HEARTBEAT_PORT   GPIOC
#define LED_HEARTBEAT_PIN    GPIO_PIN_13
#define LED_HEARTBEAT_TOGGLE() HAL_GPIO_TogglePin(LED_HEARTBEAT_PORT, LED_HEARTBEAT_PIN)
#define LED_HEARTBEAT_ON()     HAL_GPIO_WritePin(LED_HEARTBEAT_PORT, LED_HEARTBEAT_PIN, GPIO_PIN_SET)
#define LED_HEARTBEAT_OFF()    HAL_GPIO_WritePin(LED_HEARTBEAT_PORT, LED_HEARTBEAT_PIN, GPIO_PIN_RESET)

/* ------------------------------------------------------------------ */
/*  System Clock Configuration                                          */
/* ------------------------------------------------------------------ */
#define SYSTEM_CORE_CLOCK_HZ   168000000UL  /* STM32F407 max SYSCLK */

/* ------------------------------------------------------------------ */
/*  Utility Macros                                                      */
/* ------------------------------------------------------------------ */
#define BIT_SET(reg, bit)       ((reg) |=  (1U << (bit)))
#define BIT_CLEAR(reg, bit)     ((reg) &= ~(1U << (bit)))
#define BIT_CHECK(reg, bit)     (((reg) >> (bit)) & 1U)
#define ARRAY_SIZE(arr)         (sizeof(arr) / sizeof((arr)[0]))

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
