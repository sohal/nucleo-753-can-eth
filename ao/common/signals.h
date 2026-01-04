/*
 * signals.h - Signal definitions for QP/C active objects
 *
 *  Created on: Jan 4, 2026
 *      Author: QP/C Integration
 */

#pragma once

#include "qpc.h"

#ifdef __cplusplus
extern "C" {
#endif

enum AppSignals {
    DUMMY_SIG = Q_USER_SIG,  /* Reserved dummy signal */
    
    /* Published signals */
    OTA_START_SIG,           /* OTA firmware update started */
    OTA_COMPLETE_SIG,        /* OTA firmware update completed */
    MAX_PUB_SIG,             /* Last published signal */
    
    /* Active Object-specific signals */
    TIMEOUT_SIG,             /* Blinky timeout signal */
    POLL_TIMEOUT_SIG,        /* Mongoose poll timeout signal */
    STACK_CHECK_SIG,         /* Stack monitor check signal */
    
    MAX_SIG                  /* Last signal */
};

#ifdef __cplusplus
}
#endif
