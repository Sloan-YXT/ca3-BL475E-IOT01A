/*
 * hal_config.h
 *
 *  Created on: 2023年2月14日
 *      Author: utt.yao
 */

#ifndef INC_HAL_CONFIG_H_
#define INC_HAL_CONFIG_H_
#include "main.h"
#include "cyclic.h"
#include "stm32l4xx_hal_tim.h"
#include "stm32l475e_iot01_accelero.h"
#include "stm32l475e_iot01_gyro.h"
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_psensor.h"
#include "stm32l475e_iot01_magneto.h"
#include "sensor_config.h"
extern UART_HandleTypeDef huart1;
extern volatile int seconds;
void hal_Init(void);
void SystemClock_Config(void);
void timerDelay(uint16_t time);
extern TIM_HandleTypeDef TIM1_Handler;
extern __IO FlagStatus cmdDataReady;
extern SPI_HandleTypeDef hspi3;
//note this: arm cortexM3 pipeline doesn't pipeline the next 2 instructions, it pipelines next 2 half-worlds. (makes sense)
#define HEAP_BASE  0x20000000+96*0x400
#define TILT_DELAY 50
#endif /* INC_HAL_CONFIG_H_ */
