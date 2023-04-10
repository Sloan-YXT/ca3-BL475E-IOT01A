#ifndef __SENSOR_CONFIG_H
#define __SENSOR_CONFIG_H
#include "stm32l475e_iot01_accelero.h"
#include "stm32l475e_iot01_gyro.h"
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_psensor.h"
#include "stm32l475e_iot01_magneto.h"
void sensor_config(void);
void lsm6dsl_single_tap_intr_en(void);
void lsm6dsl_relative_tilt_intr_en(void);
void lsm6dsl_dready_en(void);
void lsm6dsl_dready_dis(void);
void lis3mdl_dready_en(void);
void lis3mdl_dready_dis(void);
void lps22hb_dready_en(void);
void lps22hb_dready_dis(void);
void hts221_dready_en(void);
void hts221_dready_dis(void);
int convert(float data,int integer[],int floating[]);
//enum Sensor_Index
//{
//	ACCELERO,
//	TEMP,
//	MEGNETO,
//	PIEZO,
//	GYRO,
//	HUMI,
//	WIFI
//};
enum Sensor_Index
{
	ACCELERO,
	TEMP,
	HUMI,
	WIFI
};
#endif
