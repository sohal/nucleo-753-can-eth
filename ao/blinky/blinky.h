/*
 * demo.h
 *
 *  Created on: Dec 30, 2025
 *      Author: s
 *  Modified on: Jan 4, 2026
 *      Modified: Updated for QP/C integration
 */
#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#include "qpc.h"
#include "main.h"

/* Blinky Active Object */
extern QActive * const AO_Blinky;

/* Blinky constructor */
void Blinky_ctor(void);

/* BSP functions */
void BSP_ledOn(void);
void BSP_ledOff(void);

#ifdef __cplusplus
}
#endif /* APPLICATION_DEMO_DEMO_H_ */
