/*
 * QP/C Configuration Header
 * Configuration for Quantum Platform in C for STM32H753 with QK kernel
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* QP Configuration Options */
#define QP_API_VERSION 0

/* Disable Q-SPY software tracing */
/* Note: Q_SPY should not be defined (not set to 0) when disabled */

/* Kernel selection (QK - preemptive non-blocking kernel) */
/* Note: Set via CMake configuration */

/* Memory pool configuration */
#define QF_MAX_ACTIVE 16    // Maximum number of active objects
#define QF_MAX_EPOOL 3      // Maximum number of event pools
#define QF_MAX_TICK_RATE 1  // Number of clock tick rates (1 for system tick)

/* Event queue configuration */
#define QF_EVENT_SIZ_SIZE 2 // Size of event size counter (2 bytes = up to 65535 events)

/* Time event configuration */
#define QF_TIMEEVT_CTR_SIZE 4 // Size of time event counter (4 bytes for 32-bit counter)

/* Memory pool block-size */
/* Allow for safety margin in event sizes */
#define QF_MPOOL_SIZ_SIZE 2
#define QF_MPOOL_CTR_SIZE 2

#ifdef __cplusplus
}
#endif

