/*
 * CI Stub: stm32f4xx_hal.h — minimal types for compile-check only.
 */
#ifndef STM32F4XX_HAL_H
#define STM32F4XX_HAL_H

#include <stdint.h>

typedef enum { HAL_OK = 0, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;

typedef uint32_t (*HAL_LockCpltCallbackTypeDef)(void *);

typedef struct {
    uint32_t Pin;
    uint32_t Mode;
    uint32_t Pull;
    uint32_t Speed;
    uint32_t Alternate;
} GPIO_InitTypeDef;

#define GPIO_PIN_0   ((uint32_t)0x0001U)
#define GPIO_PIN_1   ((uint32_t)0x0002U)
#define GPIO_PIN_5   ((uint32_t)0x0020U)
#define GPIO_PIN_6   ((uint32_t)0x0040U)
#define GPIO_PIN_8   ((uint32_t)0x0100U)
#define GPIO_PIN_9   ((uint32_t)0x0200U)
#define GPIO_PIN_13  ((uint32_t)0x2000U)

#define GPIO_MODE_OUTPUT_PP    0x01U
#define GPIO_MODE_INPUT        0x00U
#define GPIO_MODE_AF_PP        0x02U
#define GPIO_NOPULL            0x00U
#define GPIO_PULLDOWN          0x00U
#define GPIO_SPEED_FREQ_LOW    0x00U
#define GPIO_SPEED_FREQ_MEDIUM 0x01U
#define GPIO_SPEED_FREQ_HIGH   0x02U
#define GPIO_AF7_USART2        0x07U
#define GPIO_AF7_USART3        0x07U

#define GPIO_PIN_SET   1U
#define GPIO_PIN_RESET 0U

typedef void *USART_TypeDef;
typedef void *TIM_TypeDef;
typedef void *I2C_TypeDef;
typedef void *GPIO_TypeDef;

#define GPIOD ((GPIO_TypeDef *)1)
#define GPIOE ((GPIO_TypeDef *)2)
#define GPIOB ((GPIO_TypeDef *)3)
#define GPIOC ((GPIO_TypeDef *)4)

#define USART2 ((USART_TypeDef *)1)
#define USART3 ((USART_TypeDef *)2)
#define TIM2   ((TIM_TypeDef *)3)
#define TIM3   ((TIM_TypeDef *)4)
#define TIM4   ((TIM_TypeDef *)5)
#define I2C1   ((I2C_TypeDef *)6)

typedef struct {
    USART_TypeDef *Instance;
    volatile uint32_t SR;
    volatile uint32_t DR;
} UART_HandleTypeDef;

typedef struct {
    TIM_TypeDef *Instance;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t CNT;
    volatile uint32_t SR;
} TIM_HandleTypeDef;

typedef struct {
    I2C_TypeDef *Instance;
} I2C_HandleTypeDef;

typedef void *RCC_OscInitTypeDef_def;
typedef void *RCC_ClkInitTypeDef_def;

#define UART_FLAG_RXNE  0x0020U
#define UART_FLAG_TXE   0x0080U
#define UART_FLAG_TC    0x0040U
#define UART_FLAG_ORE   0x0008U
#define UART_FLAG_FE    0x0002U
#define UART_FLAG_NE    0x0004U

#define UART_IT_RXNE    0x0020U

#define TIM_FLAG_UPDATE 0x0001U
#define TIM_IT_UPDATE   0x0001U

#define I2C_MEMADD_SIZE_16BIT 0x02U

#define USART2_IRQn  38U
#define USART3_IRQn  39U
#define I2C1_ER_IRQn 32U
#define SysTick_IRQn (-1)

/* Dummy register access macros */
#define __HAL_RCC_PWR_CLK_ENABLE()                      do { } while(0)
#define __HAL_PWR_VOLTAGESCALING_CONFIG(x)              do { (void)(x); } while(0)
#define __HAL_RCC_GPIOB_CLK_ENABLE()                    do { } while(0)
#define __HAL_RCC_GPIOC_CLK_ENABLE()                    do { } while(0)
#define __HAL_RCC_GPIOD_CLK_ENABLE()                    do { } while(0)
#define __HAL_RCC_GPIOE_CLK_ENABLE()                    do { } while(0)
#define __HAL_UART_ENABLE_IT(huart, it)                 do { (void)(huart);(void)(it); } while(0)
#define __HAL_UART_DISABLE_IT(huart, it)                do { (void)(huart);(void)(it); } while(0)
#define __HAL_UART_CLEAR_OREFLAG(huart)                 do { (void)(huart); } while(0)
#define __HAL_TIM_SET_AUTORELOAD(htim, val)             do { (void)(htim);(void)(val); } while(0)
#define __HAL_TIM_SET_COUNTER(htim, val)                do { (void)(htim);(void)(val); } while(0)
#define __HAL_TIM_CLEAR_FLAG(htim, flg)                 do { (void)(htim);(void)(flg); } while(0)
#define __HAL_TIM_ENABLE_IT(htim, it)                   do { (void)(htim);(void)(it); } while(0)
#define __HAL_TIM_DISABLE_IT(htim, it)                  do { (void)(htim);(void)(it); } while(0)
#define __HAL_NVIC_SetPriority(irq, pri, sub)           do { (void)(irq);(void)(pri);(void)(sub); } while(0)
#define __NOP()                                         do { } while(0)

#define RCC_OSCILLATORTYPE_HSE    0x01U
#define RCC_OSCILLATORTYPE_HSI    0x02U
#define RCC_HSE_ON                0x01U
#define RCC_HSI_ON                0x01U
#define RCC_PLL_ON                0x01U
#define RCC_PLLSOURCE_HSE         0x01U
#define RCC_PLLSOURCE_HSI         0x02U
#define RCC_PLLP_DIV2             0x02U
#define RCC_SYSCLKSOURCE_PLLCLK   0x02U
#define RCC_SYSCLK_DIV1           0x00U
#define RCC_HCLK_DIV1             0x00U
#define RCC_HCLK_DIV4             0x09U
#define RCC_HCLK_DIV2             0x08U
#define RCC_CLOCKTYPE_SYSCLK      0x01U
#define RCC_CLOCKTYPE_HCLK        0x02U
#define RCC_CLOCKTYPE_PCLK1       0x04U
#define RCC_CLOCKTYPE_PCLK2       0x08U
#define PWR_REGULATOR_VOLTAGE_SCALE1 0x01U
#define FLASH_LATENCY_5           0x05U
#define SYSTICK_CLKSOURCE_HCLK    0x04U

static inline void HAL_GPIO_WritePin(GPIO_TypeDef *p, uint32_t pin, uint32_t val)
{ (void)p;(void)pin;(void)val; }
static inline uint32_t HAL_GPIO_ReadPin(GPIO_TypeDef *p, uint32_t pin)
{ (void)p;(void)pin;return 0U; }
static inline void HAL_GPIO_TogglePin(GPIO_TypeDef *p, uint32_t pin)
{ (void)p;(void)pin; }
static inline void HAL_GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *cfg)
{ (void)p;(void)cfg; }

static inline HAL_StatusTypeDef HAL_RCC_OscConfig(void *c) { (void)c;return HAL_OK; }
static inline HAL_StatusTypeDef HAL_RCC_ClockConfig(void *c, uint32_t f) { (void)c;(void)f;return HAL_OK; }
static inline uint32_t HAL_RCC_GetHCLKFreq(void) { return 168000000UL; }
static inline void HAL_SYSTICK_Config(uint32_t t) { (void)t; }
static inline void HAL_SYSTICK_CLKSourceConfig(uint32_t s) { (void)s; }

static inline HAL_StatusTypeDef HAL_I2C_Mem_Write(void *hi2c, uint32_t addr, uint32_t memaddr,
    uint32_t sz, uint8_t *data, uint16_t len, uint32_t timeout)
{ (void)hi2c;(void)addr;(void)memaddr;(void)sz;(void)data;(void)len;(void)timeout;return HAL_OK; }

static inline HAL_StatusTypeDef HAL_I2C_Mem_Read(void *hi2c, uint32_t addr, uint32_t memaddr,
    uint32_t sz, uint8_t *data, uint16_t len, uint32_t timeout)
{ (void)hi2c;(void)addr;(void)memaddr;(void)sz;(void)data;(void)len;(void)timeout;return HAL_OK; }

static inline HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim)
{ (void)htim;return HAL_OK; }
static inline HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *htim)
{ (void)htim;return HAL_OK; }

static inline void HAL_Init(void)                     { }
static inline void HAL_Delay(uint32_t ms)             { (void)ms; }
static inline void HAL_NVIC_SetPriorityGrouping(uint32_t g) { (void)g; }

#endif /* STM32F4XX_HAL_H */
