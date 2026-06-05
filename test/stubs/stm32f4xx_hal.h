/*
 * CI Stub: stm32f4xx_hal.h — minimal types for compile-check only.
 * Structs mirror the real HAL layout enough to satisfy field access.
 */
#ifndef STM32F4XX_HAL_H
#define STM32F4XX_HAL_H

#include <stdint.h>
#include <stdbool.h>

typedef enum { HAL_OK = 0, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;

/* ---- GPIO ---- */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFRL;
    volatile uint32_t AFRH;
} GPIO_TypeDef;

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

/* ---- GPIO Instances ---- */
extern GPIO_TypeDef _stub_gpiod, _stub_gpioe, _stub_gpiob, _stub_gpioc;
#define GPIOD (&_stub_gpiod)
#define GPIOE (&_stub_gpioe)
#define GPIOB (&_stub_gpiob)
#define GPIOC (&_stub_gpioc)

/* ---- USART ---- */
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

typedef struct {
    USART_TypeDef *Instance;
    uint32_t        Init;
    uint8_t        *pTxBuffPtr;
    uint16_t        TxXferSize;
    uint16_t        TxXferCount;
    uint8_t        *pRxBuffPtr;
    uint16_t        RxXferSize;
    uint16_t        RxXferCount;
} UART_HandleTypeDef;

extern USART_TypeDef _stub_usart2, _stub_usart3;
#define USART2 (&_stub_usart2)
#define USART3 (&_stub_usart3)

#define UART_FLAG_RXNE  0x0020U
#define UART_FLAG_TXE   0x0080U
#define UART_FLAG_TC    0x0040U
#define UART_FLAG_ORE   0x0008U
#define UART_FLAG_FE    0x0002U
#define UART_FLAG_NE    0x0004U
#define UART_IT_RXNE    0x0020U

/* ---- Timer ---- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t RCR;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    volatile uint32_t BDTR;
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
    volatile uint32_t OR;
} TIM_TypeDef;

typedef struct {
    TIM_TypeDef *Instance;
    uint32_t     Init;
} TIM_HandleTypeDef;

extern TIM_TypeDef _stub_tim2, _stub_tim3, _stub_tim4;
#define TIM2 (&_stub_tim2)
#define TIM3 (&_stub_tim3)
#define TIM4 (&_stub_tim4)

#define TIM_FLAG_UPDATE 0x0001U
#define TIM_IT_UPDATE   0x0001U

/* ---- I2C ---- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t DR;
    volatile uint32_t SR1;
    volatile uint32_t SR2;
    volatile uint32_t CCR;
    volatile uint32_t TRISE;
} I2C_TypeDef;

typedef struct {
    I2C_TypeDef *Instance;
} I2C_HandleTypeDef;

extern I2C_TypeDef _stub_i2c1;
#define I2C1 (&_stub_i2c1)

#define I2C_MEMADD_SIZE_16BIT 0x02U

/* ---- IRQ Numbers ---- */
#define USART2_IRQn  38U
#define USART3_IRQn  39U
#define I2C1_ER_IRQn 32U
#define SysTick_IRQn (-1)

/* ---- RCC ---- */
typedef struct {
    uint32_t OscillatorType;
    uint32_t HSEState;
    uint32_t HSIState;
    struct {
        uint32_t PLLState;
        uint32_t PLLSource;
        uint32_t PLLM;
        uint32_t PLLN;
        uint32_t PLLP;
        uint32_t PLLQ;
    } PLL;
} RCC_OscInitTypeDef;

typedef struct {
    uint32_t ClockType;
    uint32_t SYSCLKSource;
    uint32_t AHBCLKDivider;
    uint32_t APB1CLKDivider;
    uint32_t APB2CLKDivider;
} RCC_ClkInitTypeDef;

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

/* ---- HAL Macros ---- */
#define __HAL_RCC_PWR_CLK_ENABLE()              do { } while(0)
#define __HAL_PWR_VOLTAGESCALING_CONFIG(x)      do { (void)(x); } while(0)
#define __HAL_RCC_GPIOB_CLK_ENABLE()            do { } while(0)
#define __HAL_RCC_GPIOC_CLK_ENABLE()            do { } while(0)
#define __HAL_RCC_GPIOD_CLK_ENABLE()            do { } while(0)
#define __HAL_RCC_GPIOE_CLK_ENABLE()            do { } while(0)
#define __HAL_UART_ENABLE_IT(h,i)               do { (void)(h);(void)(i); } while(0)
#define __HAL_UART_DISABLE_IT(h,i)              do { (void)(h);(void)(i); } while(0)
#define __HAL_UART_CLEAR_OREFLAG(h)             do { (void)(h); } while(0)
#define __HAL_TIM_SET_AUTORELOAD(h,v)           do { (void)(h);(void)(v); } while(0)
#define __HAL_TIM_SET_COUNTER(h,v)              do { (void)(h);(void)(v); } while(0)
#define __HAL_TIM_CLEAR_FLAG(h,f)               do { (void)(h);(void)(f); } while(0)
#define __HAL_TIM_ENABLE_IT(h,i)                do { (void)(h);(void)(i); } while(0)
#define __HAL_TIM_DISABLE_IT(h,i)               do { (void)(h);(void)(i); } while(0)
#define __NOP()                                 do { } while(0)

/* ---- HAL Inline Functions ---- */
static inline void HAL_Init(void) { }

static inline void HAL_GPIO_WritePin(GPIO_TypeDef *p, uint32_t pin, uint32_t val)
{ (void)p;(void)pin;(void)val; }
static inline uint32_t HAL_GPIO_ReadPin(GPIO_TypeDef *p, uint32_t pin)
{ (void)p;(void)pin;return 0U; }
static inline void HAL_GPIO_TogglePin(GPIO_TypeDef *p, uint32_t pin)
{ (void)p;(void)pin; }
static inline void HAL_GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *cfg)
{ (void)p;(void)cfg; }

static inline HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *c) { (void)c;return HAL_OK; }
static inline HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *c, uint32_t f) { (void)c;(void)f;return HAL_OK; }
static inline uint32_t HAL_RCC_GetHCLKFreq(void) { return 168000000UL; }
static inline void HAL_SYSTICK_Config(uint32_t t) { (void)t; }
static inline void HAL_SYSTICK_CLKSourceConfig(uint32_t s) { (void)s; }
static inline void HAL_NVIC_SetPriority(int32_t irq, uint32_t pri, uint32_t sub) { (void)irq;(void)pri;(void)sub; }

static inline HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint32_t a, uint32_t ma,
    uint32_t sz, uint8_t *d, uint16_t l, uint32_t t)
{ (void)h;(void)a;(void)ma;(void)sz;(void)d;(void)l;(void)t;return HAL_OK; }
static inline HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h, uint32_t a, uint32_t ma,
    uint32_t sz, uint8_t *d, uint16_t l, uint32_t t)
{ (void)h;(void)a;(void)ma;(void)sz;(void)d;(void)l;(void)t;return HAL_OK; }

static inline HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *h) { (void)h;return HAL_OK; }
static inline HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *h) { (void)h;return HAL_OK; }

#endif /* STM32F4XX_HAL_H */
