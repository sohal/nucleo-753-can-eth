/*
 * demo.c
 *
 *  Created on: Dec 30, 2025
 *      Author: s
 *  Modified on: Jan 4, 2026
 *      Modified: Refactored for QP/C integration with Mongoose
 */
#include "blinky.h"
#include "../common/bsp.h"
#include "../common/signals.h"
#include "main.h"

/* QP/C module definition for assertions */
Q_DEFINE_THIS_MODULE("blinky")

/*
 * Blinky Active Object
 * Toggles LDGreen LED at 1Hz (500ms on, 500ms off)
 * Note: LDGreen is shared with Mongoose HTTP LED handler (concurrent control)
 */

typedef struct {
    QActive super;       /* QActive superclass (inheritance) */
    QTimeEvt timeEvt;    /* Private time event generator */
} Blinky;

/* Blinky instance and opaque pointer */
static Blinky Blinky_inst;
QActive * const AO_Blinky = &Blinky_inst.super;

/* State function prototypes */
static QState Blinky_initial(Blinky * const me, void const * const par);
static QState Blinky_off(Blinky * const me, QEvt const * const e);
static QState Blinky_on(Blinky * const me, QEvt const * const e);

/* BSP functions for LED control */
void BSP_ledOn(void) {
    HAL_GPIO_WritePin(LDGreen_GPIO_Port, LDGreen_Pin, GPIO_PIN_SET);
}

void BSP_ledOff(void) {
    HAL_GPIO_WritePin(LDGreen_GPIO_Port, LDGreen_Pin, GPIO_PIN_RESET);
}

/*==========================================================================*/
/* Blinky constructor */
void Blinky_ctor(void) {
    Blinky * const me = &Blinky_inst;
    
    /* Call superclass constructor */
    QActive_ctor(&me->super, Q_STATE_CAST(&Blinky_initial));
    
    /* Initialize time event */
    QTimeEvt_ctorX(&me->timeEvt, &me->super, TIMEOUT_SIG, 0U);
}

/*==========================================================================*/
/* Initial transition - arms timer for 1Hz blink (500ms intervals) */
static QState Blinky_initial(Blinky * const me, void const * const par) {
    (void)par;  /* Unused parameter */
    
    /* Arm the time event: 500ms intervals for 1Hz blink */
    QTimeEvt_armX(&me->timeEvt, 
                  BSP_TICKS_PER_SEC/2U,  /* 500ms initial timeout */
                  BSP_TICKS_PER_SEC/2U); /* 500ms periodic interval */
    
    return Q_TRAN(&Blinky_off);
}

/*==========================================================================*/
/* "off" state */
static QState Blinky_off(Blinky * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            BSP_ledOff();
            status = Q_HANDLED();
            break;
        }
        
        case TIMEOUT_SIG: {
            status = Q_TRAN(&Blinky_on);
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
/* "on" state */
static QState Blinky_on(Blinky * const me, QEvt const * const e) {
    QState status;
    
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            BSP_ledOn();
            status = Q_HANDLED();
            break;
        }
        
        case TIMEOUT_SIG: {
            status = Q_TRAN(&Blinky_off);
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
