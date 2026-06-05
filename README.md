# STM32 KNX/Modbus Smart Bridge

Bidirectional protocol bridge between KNX TP (Twisted Pair) and Modbus RTU (RS485), running on STM32 with FreeRTOS.

## Features

- **250 freely configurable data channels** — KNX Group Objects mapped to Modbus coils/registers with bidirectional translation, scaling, offset, and min/max clamping
- **50 logic blocks** — AND/OR/NOT/XOR, threshold comparisons, algebraic expressions, IF-THEN-ELSE conditionals
- **Predefined algorithms** — PI controller loop for temperature/humidity, Magnus-formula dew point calculation
- **KNX Data Secure** — encrypted and authenticated communication over TPUART at 9600 bps
- **Virtual Holder** — hotel room presence state machine: door sensor + PIR, multi-profile detection (Guest / Housekeeping / Maintenance / Unexpected)
- **ETS-configurable** — full application program for Engineering Tool Software
- **Persistent storage** — EEPROM-backed configuration tables with CRC validation and deferred writes
- **Thread-safe** — FreeRTOS mutexes on all shared resources; zero blocking calls in protocol tasks

## Hardware

| Interface      | Peripheral | Notes                                    |
|:---------------|:-----------|:-----------------------------------------|
| KNX TPUART     | USART2     | 9600 bps, fixed                           |
| Modbus RS485   | USART3     | 38400 bps, DE/RE pin on PE0              |
| EEPROM         | I2C1       | 64 KiB, AT24C512 layout                   |
| Digital Inputs | PB0, PB1   | Door sensor + PIR (Virtual Holder)        |
| Heartbeat LED  | PC13       |                                          |

Target: STM32F407VG (168 MHz SYSCLK). Portable to other STM32F4/G4 variants.

## Architecture

```
KNX Bus ──► TPUART ISR ──► qKnxRx ──► ┐
                                        ├──► DataBrokerTask ──┬──► qKnxTx ──► KNX Bus
Modbus ──► RS485 ISR ──► qModbusRx ──►┘ (250-channel cache)  │
     Bus                                                        └──► qModbusTx ──► Modbus Bus
                                           │
                              triggers ─► qLogicEvent ──► LogicEngineTask (50 blocks)
                              GPIO PB0/PB1 ─────────────► VirtualHolderTask
```

### FreeRTOS Task Map

| Task            | Prio | Stack (words) | Role                            |
|:---------------:|:----:|:-------------:|:--------------------------------|
| KNX_Rx          | 9    | 512           | TPUART byte reception           |
| Modbus_Rx       | 8    | 512           | RS485 frame detection           |
| KNX_Tx          | 7    | 384           | KNX telegram transmission       |
| Modbus_Tx       | 7    | 384           | RS485 frame transmission        |
| DataBroker      | 6    | 1536          | Channel mapping & dispatch      |
| LogicEngine     | 5    | 2048          | Logic blocks, PI, dew point     |
| VirtualHolder   | 4    | 768           | Hotel room presence FSM         |
| Nvram           | 3    | 640           | EEPROM deferred writes          |
| Diag            | 2    | 256           | Heartbeat, watchdog, stats      |

## Directory Structure

```
├── Core/Inc/          # Project config, task priorities, shared types, board pin map
├── Core/Src/          # main.c entry point
├── Drivers/BSP/       # TPUART, RS485, EEPROM driver wrappers
├── Middlewares/KNX/    # TP parser, Data Secure, group objects, ETS parser
├── Middlewares/Modbus/ # RTU framing stack, master/slave engines
├── App/DataBroker/     # Mapping table, translation broker
├── App/LogicEngine/    # Logic blocks, PI controller, dew point algorithm
├── App/VirtualHolder/  # Hotel room presence state machine
└── config/             # EEPROM memory layout
```

## Build

Requires an STM32 toolchain (ARM GCC) and CMake. With STM32CubeMX generated code in the project:

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake
make -j$(nproc)
```

Flash with ST-Link or J-Link:

```bash
st-flash write build/stm32_knx_bridge.bin 0x08000000
```

## Coding Standards

- MISRA C 2012 compliance via `cppcheck --addon=misra`
- No blocking calls in FreeRTOS tasks (`HAL_Delay` prohibited)
- All shared resources protected by mutexes
- Input validation on all parsed telegrams and frames
- Stack sizes validated with `uxTaskGetStackHighWaterMark()`

## License

Proprietary. All rights reserved.
