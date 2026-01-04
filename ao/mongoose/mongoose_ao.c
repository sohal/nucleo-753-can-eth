/*
 * mongoose_ao.c - Mongoose Active Object implementation
 *
 *  Created on: Jan 4, 2026
 *      Author: QP/C Integration
 */

#include "mongoose_ao.h"
#include "../common/signals.h"
#include "../../frameworks/mongoose/mongoose.h"

/* QP/C module definition for assertions */
Q_DEFINE_THIS_MODULE("mongoose_ao")

/* Mongoose manager is global from mongoose implementation */
extern struct mg_mgr g_mgr;

/* MongooseAO instance and opaque pointer */
static MongooseAO MongooseAO_inst;
QActive * const AO_Mongoose = &MongooseAO_inst.super;

/* State function prototypes */
static QState MongooseAO_initial(MongooseAO * const me, void const * const par);
static QState MongooseAO_polling(MongooseAO * const me, QEvt const * const e);

/*==========================================================================*/
/* MongooseAO constructor */
void MongooseAO_ctor(void) {
    MongooseAO * const me = &MongooseAO_inst;
    
    /* Call superclass constructor */
    QActive_ctor(&me->super, Q_STATE_CAST(&MongooseAO_initial));
    
    /* Initialize time event */
    QTimeEvt_ctorX(&me->pollTimer, &me->super, POLL_TIMEOUT_SIG, 0U);
}

/*==========================================================================*/
/* Initial state */
static QState MongooseAO_initial(MongooseAO * const me, void const * const par) {
    (void)par;  /* Unused parameter */
    
    /* Arm the polling timer: 5ms intervals (200Hz) */
    QTimeEvt_armX(&me->pollTimer, 
                  BSP_TICKS_PER_SEC/200U,  /* 5ms initial timeout */
                  BSP_TICKS_PER_SEC/200U); /* 5ms periodic interval */
    
    return Q_TRAN(&MongooseAO_polling);
}

/*==========================================================================*/
/* Polling state */
static QState MongooseAO_polling(MongooseAO * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            status = Q_HANDLED();
            break;
        }
        
        case POLL_TIMEOUT_SIG: {
            /* Poll Mongoose event manager with 0 timeout (non-blocking) */
            mg_mgr_poll(&g_mgr, 0);
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
