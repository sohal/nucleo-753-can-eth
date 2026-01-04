/*
 * mongoose_ao.h - Mongoose Active Object header
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

/* MongooseAO Active Object */
typedef struct {
    QActive super;       /* Inherit QActive */
    QTimeEvt pollTimer;  /* Time event for periodic polling */
} MongooseAO;

/* Global opaque pointer */
extern QActive * const AO_Mongoose;

/* Constructor */
void MongooseAO_ctor(void);

#ifdef __cplusplus
}
#endif
