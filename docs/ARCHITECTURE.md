# Architecture Overview

This document explains the system architecture of the nucleo-753-can-eth firmware.

## Table of Contents

- [System Architecture](#system-architecture)
- [hal753 Library Integration](#hal753-library-integration)
- [Entry Point and Initialization](#entry-point-and-initialization)
- [Active Object Pattern](#active-object-pattern)
- [Memory Layout](#memory-layout)
- [Call Chains](#call-chains)
- [Event-Driven Architecture](#event-driven-architecture)

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                  nucleo-753-can-eth                     │
│                   (This Repository)                     │
├─────────────────────────────────────────────────────────┤
│  Active Objects        │  Frameworks                    │
│  ├── Blinky           │  ├── QP/C Adapter              │
│  ├── Mongoose AO      │  │   └── application_init()   │
│  └── Stack Monitor    │  └── Mongoose Network Stack   │
└──────────────┬──────────────────────────────────────────┘
               │ Links against
┌──────────────▼──────────────────────────────────────────┐
│                   hal753 Library                        │
│          (Version from GITHUB_BRANCH_hal753)            │
├─────────────────────────────────────────────────────────┤
│  libnucleo-h753.a  │  libqpc.a  │  Headers             │
│  ├── main.c        │  ├── QK    │  ├── stm32h7xx_hal.h │
│  ├── HAL drivers   │  ├── QF    │  ├── qpc.h           │
│  ├── Peripherals   │  └── QV    │  └── BSP headers     │
│  └── BSP           │            │                       │
└─────────────────────────────────────────────────────────┘
               │ Built on
┌──────────────▼──────────────────────────────────────────┐
│         STM32CubeH7 (version from hal753 package)       │
│  ├── CMSIS (Cortex-M7 core support)                    │
│  ├── HAL Drivers (ETH, GPIO, RNG, CRYP, etc.)         │
│  └── Device Support Files                              │
└─────────────────────────────────────────────────────────┘
```

## hal753 Library Integration

The firmware consumes hal753 as a **prebuilt package** fetched via CPM:

### What hal753 Provides

1. **libnucleo-h753.a** - Static library containing:
   - `main()` function
   - STM32H7 HAL driver initialization
   - Peripheral configuration (GPIO, ETH, RNG, HASH, CRYP, etc.)
   - System clock configuration
   - Interrupt handlers

2. **libqpc.a** - QP/C framework library:
   - QK preemptive non-blocking kernel
   - Event-driven framework (QF)
   - Active Object infrastructure
   - ARM Cortex-M port

3. **Headers** - Include files for:
   - STM32H7 HAL (`stm32h7xx_hal.h`, device headers)
   - QP/C framework (`qpc.h`, `qp_config.h`)
   - Board-specific definitions (`main.h`, `stm32h7xx_hal_conf.h`)

### Dependency Fetching

Configured in `cmake/deps.nucleo-hal.cmake`:

```cmake
# Version controlled via CMakePresets/VersionPresets.json
# GITHUB_BRANCH_hal753: v1.0.1 (default)
# HAL753_VERSION extracted dynamically (v1.0.1 → 1.0.1)

CPMAddPackage(
    NAME nucleo-hal
    VERSION ${HAL753_VERSION}
    URL https://github.com/sohal/hal753/releases/download/${GITHUB_BRANCH_hal753}/nucleo-hal-${HAL753_VERSION}-gnuarm14.3.tar.gz
    URL_HASH SHA256=${HAL753_PACKAGE_SHA256}
)

find_package(nucleo ${HAL753_VERSION} REQUIRED)
target_link_libraries(nucleo-753-can-eth PRIVATE nucleo::h753)
```

**Version Management**:
- `GITHUB_BRANCH_hal753` set in `CMakePresets/VersionPresets.json`
- Package version extracted by stripping 'v' prefix
- SHA256 checksum verified during download
- Toolchain-specific packages selected via `TOOLCHAIN_VARIANT`

### Package Contents

```
nucleo-hal-<version>-<toolchain>/  # e.g., nucleo-hal-1.0.1-gnuarm14.3
├── lib/
│   ├── libnucleo-h753.a
│   ├── libqpc.a
│   └── cmake/nucleo/
│       ├── nucleoConfig.cmake
│       └── nucleoTargets.cmake
└── include/
    ├── nucleo/           # Board headers (main.h, hal_conf.h)
    ├── qpc/              # QP/C framework headers
    └── stm32cubeh7/      # STM32Cube HAL/CMSIS
```

## Entry Point and Initialization

The firmware uses **hal753's main()** as the entry point. No custom main() is provided in this repository.

### Startup Sequence

```
Reset_Handler (startup_stm32h753xx.s)
  └─> SystemInit()
      └─> __libc_init_array()
          └─> main() [from hal753 library]
              ├─> HAL_Init()
              ├─> SystemClock_Config()
              ├─> MX_GPIO_Init()
              ├─> MX_ETH_Init()
              ├─> MX_RNG_Init()
              ├─> MX_HASH_Init()
              ├─> MX_CRYP_Init()
              ├─> MX_USART3_UART_Init()
              ├─> BSP_paint_stack()      # Stack watermarking
              ├─> mongoose_init()         # Network stack init
              └─> application_init()      # ← YOUR ENTRY POINT
                  ├─> QF_init()
                  ├─> QF_poolInit()       # Event pools
                  ├─> QF_psInit()         # Pub-sub
                  ├─> Blinky_ctor()
                  ├─> MongooseAO_ctor()
                  ├─> StackMonitorAO_ctor()
                  ├─> QActive_start(...)  # Start all AOs
                  └─> QF_run()            # ← NEVER RETURNS
                      └─> QK_sched()      # QK kernel scheduler
```

### application_init() Location

Defined in `frameworks/qpc/qpc-adapter.c`:

```c
void application_init(void)
{
    QF_init();  // Initialize QP/C framework
    
    // Create event pools
    static QEvt const *smallPoolSto[20];
    static uint8_t mediumPoolSto[20][128];
    QF_poolInit(smallPoolSto, sizeof(smallPoolSto), sizeof(smallPoolSto[0]));
    QF_poolInit(mediumPoolSto, sizeof(mediumPoolSto), sizeof(mediumPoolSto[0]));
    
    // Initialize publish-subscribe
    static QSubscrList subscrSto[MAX_PUB_SIG];
    QF_psInit(subscrSto, Q_DIM(subscrSto));
    
    // Construct and start Active Objects
    Blinky_ctor();
    static QEvt const *blinkyQueueSto[20];
    QActive_start(AO_Blinky, 1U, blinkyQueueSto, Q_DIM(blinkyQueueSto),
                  (void *)0, 0U, (QEvt *)0);
    
    MongooseAO_ctor();
    static QEvt const *mongooseQueueSto[64];
    QActive_start(AO_Mongoose, 2U, mongooseQueueSto, Q_DIM(mongooseQueueSto),
                  (void *)0, 0U, (QEvt *)0);
    
    StackMonitorAO_ctor();
    static QEvt const *stackMonitorQueueSto[10];
    QActive_start(AO_StackMonitor, 3U, stackMonitorQueueSto, Q_DIM(stackMonitorQueueSto),
                  (void *)0, 0U, (QEvt *)0);
    
    QF_run();  // Transfer control to QK kernel - never returns
}
```

## Active Object Pattern

Active Objects (AOs) are event-driven, encapsulated state machines.

### AO Structure

Each AO is a hierarchical state machine with:
- **State variables** - Current state, timers
- **Event queue** - FIFO queue for incoming events
- **Priority** - Determines scheduling order
- **State handler** - Processes events based on current state

### Example: Blinky AO

```c
// ao/blinky/blinky.c
typedef struct {
    QActive super;      // Inherit from QActive
    QTimeEvt timer;     // Periodic timer for LED toggle
} BlinkyAO;

static BlinkyAO l_blinky;  // Singleton instance

void Blinky_ctor(void) {
    BlinkyAO *me = &l_blinky;
    QActive_ctor(&me->super, Q_STATE_CAST(&Blinky_initial));
    QTimeEvt_ctorX(&me->timer, &me->super, BLINKY_TIMEOUT_SIG, 0U);
}

QState Blinky_active(BlinkyAO *me, QEvt const *e) {
    switch (e->sig) {
        case Q_ENTRY_SIG:
            QTimeEvt_armX(&me->timer, BSP_TICKS_PER_SEC / 2, 0U);  // 500ms
            return Q_RET_HANDLED;
        
        case BLINKY_TIMEOUT_SIG:
            BSP_ledToggle();  // Toggle LED
            return Q_RET_HANDLED;
    }
    return Q_SUPER(&QHsm_top);
}
```

### AO Priorities

QK uses priority-based preemptive scheduling:

| Priority | AO | Notes |
|----------|----|-|
| 1 | Blinky | Lowest priority, background task |
| 2 | Mongoose | Network polling, moderate priority |
| 3 | Stack Monitor | Periodic safety check |
| **4+** | **Your custom AOs** | Use priorities 4 and higher |

Higher priority AOs preempt lower priority ones.

## Memory Layout

Defined in `linker/STM32H753XX_FLASH.ld`:

```
STM32H753 Memory Map:
┌────────────────────┬─────────────┬────────────┐
│ Region             │ Start       │ Size       │
├────────────────────┼─────────────┼────────────┤
│ FLASH (ROM)        │ 0x08000000  │ 2048 KB    │
│ DTCMRAM            │ 0x20000000  │ 128 KB     │
│ RAM_D1 (AXI SRAM)  │ 0x24000000  │ 512 KB     │
│ RAM_D2 (AHB SRAM)  │ 0x30000000  │ 288 KB     │
│ RAM_D3 (AHB SRAM)  │ 0x38000000  │ 64 KB      │
│ ITCMRAM            │ 0x00000000  │ 64 KB      │
└────────────────────┴─────────────┴────────────┘

Typical Firmware Layout:
┌────────────────────┬─────────────┬────────────┐
│ Section            │ Location    │ Typical    │
├────────────────────┼─────────────┼────────────┤
│ .text (code)       │ FLASH       │ ~100 KB    │
│ .rodata (const)    │ FLASH       │ ~10 KB     │
│ .data (init vars)  │ FLASH→RAM   │ ~2 KB      │
│ .bss (zero vars)   │ RAM         │ ~8 KB      │
│ .heap              │ RAM         │ 16 KB      │
│ .stack             │ RAM         │ 64 KB      │
│ Event pools        │ RAM         │ ~3 KB      │
│ AO queues          │ RAM         │ ~1 KB      │
│ Mongoose buffers   │ RAM         │ ~40 KB     │
└────────────────────┴─────────────┴────────────┘
```

See [MEMORY_OPTIMIZATION.md](MEMORY_OPTIMIZATION.md) for optimization strategies.

## Call Chains

### LED Toggle (Blinky AO)

```
SysTick_Handler (1kHz)
  └─> QK_ISR_ENTRY()
      └─> QF_tickX_(0)
          └─> QTimeEvt_arm() expires
              └─> Post BLINKY_TIMEOUT_SIG to Blinky queue
                  └─> QK_ISR_EXIT()
                      └─> QK_sched()
                          └─> Dispatch event to Blinky AO
                              └─> Blinky_active(BLINKY_TIMEOUT_SIG)
                                  └─> BSP_ledToggle()
                                      └─> HAL_GPIO_TogglePin()
```

### Network Polling (Mongoose AO)

```
SysTick_Handler (1kHz) - every 5ms
  └─> QK_ISR_ENTRY()
      └─> QF_tickX_(0)
          └─> MongooseAO timer expires (5ms period)
              └─> Post MONGOOSE_POLL_SIG
                  └─> QK_ISR_EXIT()
                      └─> QK_sched()
                          └─> Dispatch to MongooseAO (priority 2)
                              └─> MongooseAO_active(MONGOOSE_POLL_SIG)
                                  └─> mg_mgr_poll(&mgr, 0)
                                      └─> Handle network I/O
                                          └─> HTTP request processing
```

### Stack Monitoring

```
SysTick_Handler (1kHz) - every 10 seconds
  └─> QK_ISR_ENTRY()
      └─> QF_tickX_(0)
          └─> StackMonitorAO timer expires
              └─> Post STACK_CHECK_SIG
                  └─> QK_ISR_EXIT()
                      └─> QK_sched()
                          └─> StackMonitorAO_active(STACK_CHECK_SIG)
                              └─> BSP_check_stack_watermark()
                                  └─> Search for 0xDEADBEEF pattern
                                      └─> Calculate usage percentage
                                          └─> Assert if > 90%
```

## Event-Driven Architecture

### Signal Definitions

Defined in `ao/common/signals.h`:

```c
enum AppSignals {
    BLINKY_TIMEOUT_SIG = Q_USER_SIG,  // Blinky timer
    MONGOOSE_POLL_SIG,                 // Mongoose poll timer
    STACK_CHECK_SIG,                   // Stack monitor timer
    
    MAX_PUB_SIG,     // Published signals (none currently)
    MAX_SIG          // Total number of signals
};
```

### Event Flow

```
Timer Expiry → SysTick ISR → QF_tickX_() → Post Event → AO Queue → QK Dispatch → AO State Handler
```

### QK Kernel Scheduling

QK is a **preemptive, non-blocking** kernel:
- No context switching overhead (shares main stack)
- No per-AO stack allocation
- Priority-based scheduling
- Events processed to completion (run-to-completion)

## Build and Link Architecture

```
CMakeLists.txt
  └─> add_executable(nucleo-753-can-eth
      ├── ao/blinky/blinky.c
      ├── ao/mongoose/mongoose_ao.c
      ├── ao/stack_monitor/stack_monitor_ao.c
      ├── frameworks/qpc/qpc-adapter.c
      ├── frameworks/mongoose/mongoose.c
      ├── frameworks/mongoose/mongoose_glue.c
      ├── frameworks/mongoose/mongoose_impl.c
      ├── frameworks/mongoose/mongoose_fs.c
      └── linker/startup_stm32h753xx.s
      
  └─> target_link_libraries(nucleo::h753)
      ├── Links: libnucleo-h753.a
      │   └── Provides: main(), HAL drivers, peripherals
      └── Links: libqpc.a (transitively)
          └── Provides: QK, QF, QV, QActive
      
  └─> target_link_options(-T linker/STM32H753XX_FLASH.ld)
      └── Memory layout, section placement
```

## Next Steps

- [Extend with custom Active Objects](EXTENDING_ACTIVE_OBJECTS.md)
- [Optimize memory usage](MEMORY_OPTIMIZATION.md)
- [Debug with GDB](DEBUGGING.md)
