/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : exam_instruments.h
 * @brief          : Exam instruments firmware header
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

#ifndef __EXAM_INSTRUMENTS_H__
#define __EXAM_INSTRUMENTS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/* Public defines ------------------------------------------------------------*/
#define EXAM_SQUARE_WAVE_FREQ_HZ    1000    // 1 kHz square wave
#define EXAM_VERSION_STRING         "EXAM-1.0.0"

/* Function prototypes -------------------------------------------------------*/
void exam_instruments_init(void);
void exam_instruments_run(void);

#ifdef __cplusplus
}
#endif

#endif /* __EXAM_INSTRUMENTS_H__ */
