# Extending Active Objects

This guide shows how to create custom Active Objects (AOs) and integrate them into the firmware.

## Table of Contents

- [Active Object Basics](#active-object-basics)
- [Creating a Custom AO](#creating-a-custom-ao)
- [Signal Definitions](#signal-definitions)
- [State Machine Design](#state-machine-design)
- [Registering Your AO](#registering-your-ao)
- [Priority Assignment](#priority-assignment)
- [Queue Sizing](#queue-sizing)
- [Complete Example](#complete-example)

## Active Object Basics

An Active Object is an event-driven, encapsulated component that:
- Runs concurrently with other AOs
- Communicates via asynchronous events
- Maintains its own state machine
- Has a dedicated event queue and priority

### AO Structure

```c
typedef struct {
    QActive super;           // Inherit from QActive base class
    QTimeEvt myTimer;        // Time event for periodic tasks
    uint32_t myData;         // AO-specific data members
    // ... more state variables
} MyAO;
```

## Creating a Custom AO

### Step 1: Create AO Files

Create a new directory under `ao/`:

```bash
mkdir -p ao/my_custom_ao
cd ao/my_custom_ao
```

### Step 2: Header File (`my_custom_ao.h`)

```c
#ifndef MY_CUSTOM_AO_H
#define MY_CUSTOM_AO_H

#include "qpc.h"

// Constructor
void MyCustomAO_ctor(void);

// Global AO pointer for posting events
extern QActive * const AO_MyCustom;

#endif // MY_CUSTOM_AO_H
```

### Step 3: Implementation File (`my_custom_ao.c`)

```c
#include "my_custom_ao.h"
#include "bsp.h"
#include "signals.h"

// AO structure definition
typedef struct {
    QActive super;          // Inherit QActive
    QTimeEvt timer;         // Periodic timer
    uint32_t counter;       // Example state variable
} MyCustomAO;

// Static instance (singleton pattern)
static MyCustomAO l_myCustomAO;

// Expose AO pointer globally
QActive * const AO_MyCustom = &l_myCustomAO.super;

// Forward declarations of state handlers
static QState MyCustomAO_initial(MyCustomAO * const me, QEvt const * const e);
static QState MyCustomAO_active(MyCustomAO * const me, QEvt const * const e);

// Constructor
void MyCustomAO_ctor(void) {
    MyCustomAO *me = &l_myCustomAO;

    // Call QActive constructor
    QActive_ctor(&me->super, Q_STATE_CAST(&MyCustomAO_initial));

    // Initialize timer event
    QTimeEvt_ctorX(&me->timer, &me->super, MY_CUSTOM_TIMEOUT_SIG, 0U);

    // Initialize state variables
    me->counter = 0;
}

// Initial pseudostate - runs once on AO start
static QState MyCustomAO_initial(MyCustomAO * const me, QEvt const * const e) {
    (void)e;  // Unused parameter

    // Subscribe to published signals if needed
    // QActive_subscribe(&me->super, SOME_PUBLISHED_SIG);

    // Transition to active state
    return Q_TRAN(&MyCustomAO_active);
}

// Active state - handles events
static QState MyCustomAO_active(MyCustomAO * const me, QEvt const * const e) {
    QState status;

    switch (e->sig) {
        case Q_ENTRY_SIG: {
            // Arm timer on entry: 1 second periodic
            QTimeEvt_armX(&me->timer, BSP_TICKS_PER_SEC, BSP_TICKS_PER_SEC);
            status = Q_RET_HANDLED;
            break;
        }

        case Q_EXIT_SIG: {
            // Disarm timer on exit
            QTimeEvt_disarm(&me->timer);
            status = Q_RET_HANDLED;
            break;
        }

        case MY_CUSTOM_TIMEOUT_SIG: {
            // Handle periodic timeout
            me->counter++;

            // Example: Do something every 10 seconds
            if ((me->counter % 10) == 0) {
                // Publish an event, call a function, etc.
                QF_PUBLISH(Q_NEW(QEvt, MY_CUSTOM_EVENT_SIG), me);
            }

            status = Q_RET_HANDLED;
            break;
        }

        case MY_CUSTOM_EVENT_SIG: {
            // Handle custom event
            // Process the event...
            status = Q_RET_HANDLED;
            break;
        }

        default: {
            // Delegate unhandled events to superstate
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }

    return status;
}
```

## Signal Definitions

Add your signals to `ao/common/signals.h`:

```c
enum AppSignals {
    BLINKY_TIMEOUT_SIG = Q_USER_SIG,
    MONGOOSE_POLL_SIG,
    STACK_CHECK_SIG,

    // Your custom signals
    MY_CUSTOM_TIMEOUT_SIG,      // Timer for your AO
    MY_CUSTOM_EVENT_SIG,        // Custom event
    MY_CUSTOM_DATA_READY_SIG,   // Data ready event

    MAX_PUB_SIG,    // Last published signal (if any)
    MAX_SIG         // Total number of signals
};
```

### Signal Categories

- **Private signals** - Used only within your AO
- **Published signals** - Broadcast to multiple subscribers
  - Must be < `MAX_PUB_SIG`
  - Use `QF_PUBLISH()` to send
  - Subscribers use `QActive_subscribe()`

## State Machine Design

### Simple State Machine

```c
static QState MyAO_initial(MyAO *me, QEvt const *e) {
    // One-time initialization
    return Q_TRAN(&MyAO_active);
}

static QState MyAO_active(MyAO *me, QEvt const *e) {
    // Handle all events in one state
    switch (e->sig) {
        case MY_EVENT_SIG:
            // Handle event
            return Q_RET_HANDLED;
    }
    return Q_SUPER(&QHsm_top);
}
```

### Hierarchical State Machine

```c
// Top-level state
static QState MyAO_running(MyAO *me, QEvt const *e) {
    switch (e->sig) {
        case STOP_SIG:
            return Q_TRAN(&MyAO_stopped);
        case COMMON_EVENT_SIG:
            // Handled in parent state
            return Q_RET_HANDLED;
    }
    return Q_SUPER(&QHsm_top);
}

// Sub-state 1
static QState MyAO_idle(MyAO *me, QEvt const *e) {
    switch (e->sig) {
        case START_SIG:
            return Q_TRAN(&MyAO_busy);
    }
    return Q_SUPER(&MyAO_running);  // Parent state
}

// Sub-state 2
static QState MyAO_busy(MyAO *me, QEvt const *e) {
    switch (e->sig) {
        case DONE_SIG:
            return Q_TRAN(&MyAO_idle);
    }
    return Q_SUPER(&MyAO_running);  // Parent state
}
```

## Registering Your AO

Modify `frameworks/qpc/qpc-adapter.c`:

### Step 1: Include Your Header

```c
#include "my_custom_ao.h"
```

### Step 2: Add to application_init()

```c
void application_init(void)
{
    QF_init();

    // ... existing event pool and pub-sub init ...

    // Existing AOs
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

    // YOUR NEW AO - Priority 4
    MyCustomAO_ctor();
    static QEvt const *myCustomQueueSto[16];
    QActive_start(AO_MyCustom, 4U, myCustomQueueSto, Q_DIM(myCustomQueueSto),
                  (void *)0, 0U, (QEvt *)0);

    QF_run();
}
```

### Step 3: Update CMakeLists.txt

Add your source file to the executable:

```cmake
add_executable(${PROJECT_NAME}
    # Active Objects
    ao/blinky/blinky.c
    ao/mongoose/mongoose_ao.c
    ao/stack_monitor/stack_monitor_ao.c
    ao/my_custom_ao/my_custom_ao.c        # ← ADD THIS

    # ... rest of sources
)

# Add include directory
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ao/common
        ${CMAKE_CURRENT_SOURCE_DIR}/ao/blinky
        ${CMAKE_CURRENT_SOURCE_DIR}/ao/mongoose
        ${CMAKE_CURRENT_SOURCE_DIR}/ao/stack_monitor
        ${CMAKE_CURRENT_SOURCE_DIR}/ao/my_custom_ao    # ← ADD THIS
        # ... rest of includes
)
```

## Priority Assignment

QK uses priority-based preemptive scheduling.

### Priority Guidelines

| Priority Range | Usage |
|----------------|-------|
| 1-3 | Reserved (existing AOs) |
| **4-10** | **Application AOs** |
| 11-20 | Time-critical AOs |
| 21-31 | Interrupt-like AOs (rare) |

**Rules:**
- Lower number = lower priority
- Higher priority AOs preempt lower priority ones
- AOs of same priority share time cooperatively
- Maximum priority: 63 (configurable in `qp_config.h`)

### Example Priority Assignment

```c
QActive_start(AO_Blinky, 1U, ...);         // Lowest (background)
QActive_start(AO_Mongoose, 2U, ...);       // Network I/O
QActive_start(AO_StackMonitor, 3U, ...);   // Periodic safety check
QActive_start(AO_MyCustom, 4U, ...);       // Your application logic
QActive_start(AO_CANHandler, 5U, ...);     // CAN bus processing
QActive_start(AO_DataLogger, 6U, ...);     // Data logging
```

## Queue Sizing

Each AO has an event queue. Size it based on expected event rate.

### Queue Size Guidelines

```c
// Small queue (10-20 events): Slow, periodic AOs
static QEvt const *myQueueSto[10];
QActive_start(AO_MyAO, 4U, myQueueSto, Q_DIM(myQueueSto), ...);

// Medium queue (20-64 events): Moderate event rate
static QEvt const *myQueueSto[32];
QActive_start(AO_MyAO, 5U, myQueueSto, Q_DIM(myQueueSto), ...);

// Large queue (64+ events): High-frequency events (network, UART)
static QEvt const *myQueueSto[128];
QActive_start(AO_MyAO, 6U, myQueueSto, Q_DIM(myQueueSto), ...);
```

### Queue Overflow

If queue fills up, posting fails. Handle with:
- Increase queue size
- Process events faster
- Use `QActive_post_()` which asserts on overflow
- Use `QActive_postLIFO_()` for high-priority events

## Complete Example: CAN Handler AO

### ao/can_handler/can_handler.h

```c
#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include "qpc.h"

// Events
typedef struct {
    QEvt super;
    uint32_t id;
    uint8_t data[8];
    uint8_t len;
} CANRxEvt;

// Constructor
void CANHandler_ctor(void);

// Global AO pointer
extern QActive * const AO_CANHandler;

#endif
```

### ao/can_handler/can_handler.c

```c
#include "can_handler.h"
#include "bsp.h"
#include "signals.h"

typedef struct {
    QActive super;
    QTimeEvt pollTimer;
    uint32_t rxCount;
} CANHandlerAO;

static CANHandlerAO l_canHandler;
QActive * const AO_CANHandler = &l_canHandler.super;

static QState CANHandler_initial(CANHandlerAO *me, QEvt const *e);
static QState CANHandler_active(CANHandlerAO *me, QEvt const *e);

void CANHandler_ctor(void) {
    CANHandlerAO *me = &l_canHandler;
    QActive_ctor(&me->super, Q_STATE_CAST(&CANHandler_initial));
    QTimeEvt_ctorX(&me->pollTimer, &me->super, CAN_POLL_SIG, 0U);
    me->rxCount = 0;
}

static QState CANHandler_initial(CANHandlerAO *me, QEvt const *e) {
    (void)e;
    return Q_TRAN(&CANHandler_active);
}

static QState CANHandler_active(CANHandlerAO *me, QEvt const *e) {
    switch (e->sig) {
        case Q_ENTRY_SIG:
            // Poll CAN every 10ms
            QTimeEvt_armX(&me->pollTimer, BSP_TICKS_PER_SEC / 100,
                          BSP_TICKS_PER_SEC / 100);
            return Q_RET_HANDLED;

        case CAN_POLL_SIG:
            // Check for CAN messages
            // HAL_CAN_GetRxMessage(...);
            // If message received, publish event
            return Q_RET_HANDLED;

        case CAN_RX_SIG: {
            CANRxEvt const *rx = (CANRxEvt const *)e;
            me->rxCount++;
            // Process CAN message
            // Handle based on rx->id, rx->data, rx->len
            return Q_RET_HANDLED;
        }
    }
    return Q_SUPER(&QHsm_top);
}
```

### Update signals.h

```c
enum AppSignals {
    // ... existing signals ...
    CAN_POLL_SIG,
    CAN_RX_SIG,
    // ...
};
```

### Update CMakeLists.txt

```cmake
add_executable(${PROJECT_NAME}
    # ... existing sources ...
    ao/can_handler/can_handler.c
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        # ... existing includes ...
        ${CMAKE_CURRENT_SOURCE_DIR}/ao/can_handler
)
```

### Register in qpc-adapter.c

```c
#include "can_handler.h"

void application_init(void) {
    // ... existing initialization ...

    CANHandler_ctor();
    static QEvt const *canQueueSto[32];
    QActive_start(AO_CANHandler, 5U, canQueueSto, Q_DIM(canQueueSto),
                  (void *)0, 0U, (QEvt *)0);

    QF_run();
}
```

## Best Practices

1. **One AO per peripheral** - CAN, UART, SPI each get their own AO
2. **Keep state handlers short** - Offload heavy work to helper functions
3. **Use time events for periodic tasks** - Don't block or poll
4. **Publish events for broadcast** - Use for notifications (button press, sensor data)
5. **Direct post for point-to-point** - Use `QActive_post_()` for specific AO
6. **Allocate events from pools** - Use `Q_NEW()` for dynamic events
7. **Test queue sizes** - Monitor with QP Spy if queue overflows occur

## Debugging Tips

- **QP Spy**: Enable QS (QP Spy) for runtime tracing
- **Assertions**: QP asserts on queue overflow, invalid transitions
- **State logging**: Add `printf()` in entry/exit actions during debug
- **Priority inversion**: If low-priority AO blocks high-priority, adjust priorities

## Next Steps

- [Understand system architecture](ARCHITECTURE.md)
- [Optimize memory usage](MEMORY_OPTIMIZATION.md)
- [Debug with GDB](DEBUGGING.md)
