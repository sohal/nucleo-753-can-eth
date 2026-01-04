/*
 * bsp.h - Board Support Package for STM32H753 Nucleo
 *
 *  Created on: Jan 4, 2026
 *      Author: QP/C Integration
 */

#pragma once

#include "qpc.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BSP timing configuration */
#define BSP_TICKS_PER_SEC    1000U  /* 1ms tick resolution */

/* BSP functions */
void BSP_init(void);
void BSP_ledOn(void);
void BSP_ledOff(void);
void BSP_paint_stack(void);

#ifdef __cplusplus
}
#endif
