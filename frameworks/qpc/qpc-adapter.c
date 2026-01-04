/*
 * qpc-adapter.c
 *
 *  Created on: Dec 30, 2025
 *      Author: s
 *  Modified on: Jan 4, 2026
 *      Modified: Full QP/C framework initialization with Mongoose integration
 */

#include "qpc.h"
#include "../../ao/common/bsp.h"
#include "../../ao/common/signals.h"
#include "main.h"
#include "../../ao/mongoose/mongoose_ao.h"
#include "../../ao/stack_monitor/stack_monitor_ao.h"
#include "../../ao/blinky/blinky.h"

/* QP/C module definition for assertions */
Q_DEFINE_THIS_MODULE("qpc_adapter")

/* Stack watermark pattern */
#define STACK_PATTERN 0xDEADBEEF
#define STACK_TOTAL_SIZE 65536U

/* External stack symbols from linker script */
extern uint32_t _estack;

/* Event queue storage for active objects */
static QEvtPtr blinkyQueueSto[20];
static QEvtPtr mongooseQueueSto[64];
static QEvtPtr stackMonitorQueueSto[10];

/* Event pool storage */
static QF_MPOOL_EL(QEvt) smlPoolSto[20];      /* Small event pool */
static QF_MPOOL_EL(QEvt) medPoolSto[20];      /* Medium event pool (128 bytes) */
static uint8_t medPoolBuf[20 * 128];          /* Medium pool buffer */

/* Publish-subscribe storage */
static QSubscrList subscrSto[MAX_PUB_SIG];

/* SysTick handler for QP/C */
static void const *l_SysTick_Handler = (void const *)0;

//============================================================================
/* Stack watermark painting - must be called before main() uses stack */
void BSP_paint_stack(void) {
    uint32_t *stack_bottom = (uint32_t *)((uint32_t)&_estack - STACK_TOTAL_SIZE);
    uint32_t *stack_top = (uint32_t *)&_estack;
    uint32_t *current_sp;
    
    /* Get current stack pointer */
    __asm volatile ("mov %0, sp" : "=r" (current_sp));
    
    /* Paint from bottom to current SP with watermark pattern */
    for (uint32_t *p = stack_bottom; p < current_sp; p++) {
        *p = STACK_PATTERN;
    }
}

//============================================================================
/* QP/C application initialization - called from main() */
void application_init(void) {
    /* Initialize QF framework */
    QF_init();
    
    /* Initialize event pools */
    QF_poolInit(smlPoolSto, sizeof(smlPoolSto), sizeof(smlPoolSto[0]));
    QF_poolInit(medPoolBuf, sizeof(medPoolBuf), 128U);
    
    /* Initialize publish-subscribe */
    QActive_psInit(subscrSto, Q_DIM(subscrSto));
    
    /* Construct and start Blinky AO (Priority 1 - LED heartbeat) */
    Blinky_ctor();
    QActive_start(AO_Blinky,
        1U,                          /* QP priority */
        blinkyQueueSto,              /* Event queue storage */
        Q_DIM(blinkyQueueSto),       /* Queue length */
        (void *)0, 0U,               /* No stack (QK shares main stack) */
        (void *)0);                  /* No initialization parameter */
    
    /* Construct and start Mongoose AO (Priority 2 - Network polling) */
    MongooseAO_ctor();
    QActive_start(AO_Mongoose,
        2U,                          /* QP priority */
        mongooseQueueSto,            /* Event queue storage */
        Q_DIM(mongooseQueueSto),     /* Queue length (64 entries) */
        (void *)0, 0U,               /* No stack (QK shares main stack) */
        (void *)0);                  /* No initialization parameter */
    
    /* Construct and start Stack Monitor AO (Priority 3 - Monitoring) */
    StackMonitorAO_ctor();
    QActive_start(AO_StackMonitor,
        3U,                          /* QP priority */
        stackMonitorQueueSto,        /* Event queue storage */
        Q_DIM(stackMonitorQueueSto), /* Queue length */
        (void *)0, 0U,               /* No stack (QK shares main stack) */
        (void *)0);                  /* No initialization parameter */
    
    /* Transfer control to QK kernel - this function never returns */
    QF_run();
}

//============================================================================
/* QF callbacks */

void QF_onStartup(void) {
    /* Configure SysTick timer for BSP_TICKS_PER_SEC rate (1000Hz = 1ms) */
    SysTick_Config(SystemCoreClock / BSP_TICKS_PER_SEC);
    
    /* Assign all priority bits for preemption-prio. and none to sub-prio. */
    NVIC_SetPriorityGrouping(0U);
    
    /* Set SysTick priority using QP/C recommended priority */
    NVIC_SetPriority(SysTick_IRQn, QF_AWARE_ISR_CMSIS_PRI + 1U);
}

void QF_onCleanup(void) {
    /* Cleanup code if needed */
}

void QK_onIdle(void) {
    /* Idle processing */
#ifdef NDEBUG
    /* Put CPU to sleep in release builds */
    __WFI();
#endif
}

//============================================================================
/* Error handler and ISRs */

Q_NORETURN Q_onError(char const * const module, int_t const id) {
    /* NOTE: this implementation of the assertion handler is intended only
     * for debugging and MUST be changed for deployment of the application
     * (assuming that you ship your production code with assertions enabled).
     */
    (void)module;
    (void)id;

#ifndef NDEBUG
    /* For debugging, hang on in an endless loop */
    for (;;) {
        /* Turn on all LEDs to indicate error */
        HAL_GPIO_WritePin(LDGreen_GPIO_Port, LDGreen_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LDYellow_GPIO_Port, LDYellow_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LDRed_GPIO_Port, LDRed_Pin, GPIO_PIN_SET);
    }
#endif

    NVIC_SystemReset();
}

void SysTick_Handler(void); /* Prototype */

void SysTick_Handler(void) {
    QK_ISR_ENTRY();   /* Inform QK about entering an ISR */

    QTIMEEVT_TICK_X(0U, &l_SysTick_Handler); /* Process time events for rate 0 */

    QK_ISR_EXIT();  /* Inform QK about exiting an ISR */
}
