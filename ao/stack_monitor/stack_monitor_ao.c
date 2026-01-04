/*
 * stack_monitor_ao.c - Stack Monitor Active Object implementation
 *
 *  Created on: Jan 4, 2026
 *      Author: QP/C Integration
 */

#include "stack_monitor_ao.h"
#include "../common/signals.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* QP/C module definition for assertions */
Q_DEFINE_THIS_MODULE("stack_monitor")

/* Stack watermark pattern */
#define STACK_PATTERN 0xDEADBEEF

/* Stack monitoring configuration */
#define STACK_TOTAL_SIZE     65536U  /* 64KB stack */
#define STACK_ALERT_PERCENT  90U     /* Alert at 90% usage */
#define STACK_ALERT_BYTES    ((STACK_TOTAL_SIZE * STACK_ALERT_PERCENT) / 100U)

/* External stack symbols from linker script */
extern uint32_t _estack;
extern uint32_t _Min_Stack_Size;

/* StackMonitorAO instance and opaque pointer */
static StackMonitorAO StackMonitorAO_inst;
QActive * const AO_StackMonitor = &StackMonitorAO_inst.super;

/* State function prototypes */
static QState StackMonitorAO_initial(StackMonitorAO * const me, void const * const par);
static QState StackMonitorAO_monitoring(StackMonitorAO * const me, QEvt const * const e);

/* Helper functions */
static uint32_t check_stack_usage(void);
static void log_stack_usage(uint32_t usage);

/*==========================================================================*/
/* StackMonitorAO constructor */
void StackMonitorAO_ctor(void) {
    StackMonitorAO * const me = &StackMonitorAO_inst;
    
    /* Call superclass constructor */
    QActive_ctor(&me->super, Q_STATE_CAST(&StackMonitorAO_initial));
    
    /* Initialize time event */
    QTimeEvt_ctorX(&me->checkTimer, &me->super, STACK_CHECK_SIG, 0U);
    
    /* Initialize peak usage */
    me->peakUsage = 0;
}

/*==========================================================================*/
/* Initial state */
static QState StackMonitorAO_initial(StackMonitorAO * const me, void const * const par) {
    (void)par;  /* Unused parameter */
    
    /* Arm the check timer: 10 second intervals */
    QTimeEvt_armX(&me->checkTimer, 
                  BSP_TICKS_PER_SEC * 10U,  /* 10s initial timeout */
                  BSP_TICKS_PER_SEC * 10U); /* 10s periodic interval */
    
    /* Subscribe to OTA events */
    QActive_subscribe(&me->super, OTA_START_SIG);
    QActive_subscribe(&me->super, OTA_COMPLETE_SIG);
    
    return Q_TRAN(&StackMonitorAO_monitoring);
}

/*==========================================================================*/
/* Monitoring state */
static QState StackMonitorAO_monitoring(StackMonitorAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            status = Q_HANDLED();
            break;
        }
        
        case STACK_CHECK_SIG: {
            /* Periodic stack check */
            uint32_t usage = check_stack_usage();
            if (usage > me->peakUsage) {
                me->peakUsage = usage;
            }
            log_stack_usage(usage);
            
            /* Assert if exceeds 90% */
            Q_ASSERT(usage < STACK_ALERT_BYTES);
            
            status = Q_HANDLED();
            break;
        }
        
        case OTA_START_SIG: {
            /* Log stack usage at OTA start */
            uint32_t usage = check_stack_usage();
            const char msg[] = "\r\n=== OTA START ===\r\n";
            HAL_UART_Transmit(&huart3, (uint8_t*)msg, sizeof(msg)-1, HAL_MAX_DELAY);
            log_stack_usage(usage);
            status = Q_HANDLED();
            break;
        }
        
        case OTA_COMPLETE_SIG: {
            /* Log stack usage at OTA completion */
            uint32_t usage = check_stack_usage();
            const char msg[] = "\r\n=== OTA COMPLETE ===\r\n";
            HAL_UART_Transmit(&huart3, (uint8_t*)msg, sizeof(msg)-1, HAL_MAX_DELAY);
            log_stack_usage(usage);
            status = Q_HANDLED();
            break;
        }
        
        case Q_EXIT_SIG: {
            status = Q_HANDLED();
            break;
        }
        
        default: {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    
    return status;
}

/*==========================================================================*/
/* Helper: Check stack usage by scanning for watermark pattern */
static uint32_t check_stack_usage(void) {
    uint32_t *stack_bottom = (uint32_t *)((uint32_t)&_estack - STACK_TOTAL_SIZE);
    uint32_t *p = stack_bottom;
    
    /* Scan from bottom until we find non-pattern data */
    while (p < (uint32_t*)&_estack && *p == STACK_PATTERN) {
        p++;
    }
    
    /* Calculate used bytes */
    uint32_t unused_bytes = (uint32_t)((uint32_t)p - (uint32_t)stack_bottom);
    uint32_t used_bytes = STACK_TOTAL_SIZE - unused_bytes;
    
    return used_bytes;
}

/*==========================================================================*/
/* Helper: Log stack usage to UART in human-readable format */
static void log_stack_usage(uint32_t usage) {
    char buffer[80];
    uint32_t percent = (usage * 100) / STACK_TOTAL_SIZE;
    uint32_t percent_decimal = ((usage * 1000) / STACK_TOTAL_SIZE) % 10;
    
    int len = snprintf(buffer, sizeof(buffer), 
                       "Stack: %lu/%u bytes (%lu.%lu%%)\r\n",
                       usage, STACK_TOTAL_SIZE, percent, percent_decimal);
    
    if (len > 0 && len < (int)sizeof(buffer)) {
        HAL_UART_Transmit(&huart3, (uint8_t*)buffer, len, HAL_MAX_DELAY);
    }
}
