/*
 * stack_monitor_ao.h - Stack Monitor Active Object header
 *
 *  Created on: Jan 4, 2026
 *      Author: QP/C Integration
 */

#pragma once

#include "qpc.h"
#include "../common/bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* StackMonitorAO Active Object */
typedef struct {
    QActive super;           /* Inherit QActive */
    QTimeEvt checkTimer;     /* Time event for periodic stack checking */
    uint32_t peakUsage;      /* Peak stack usage in bytes */
} StackMonitorAO;

/* Global opaque pointer */
extern QActive * const AO_StackMonitor;

/* Constructor */
void StackMonitorAO_ctor(void);

#ifdef __cplusplus
}
#endif
