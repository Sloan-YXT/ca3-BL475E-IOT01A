#include "sensor_config.h"
//set the sensor slope interrupt to implement event driven in hardware way, which is better I think
void lsm6dsl_single_tap_intr_en(void)
{
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_TAP_CFG, 0b10001110);
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_TAP_THS_6D, 0b10);
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_INT_DUR2, 0x06);
	uint8_t tmp = SENSOR_IO_Read(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_MD1_CFG);
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_MD1_CFG, tmp|0x40);
}
void lsm6dsl_relative_tilt_intr_en(void)
{
	uint8_t tmp;
	tmp = SENSOR_IO_Read(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_CTRL10_C);
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_CTRL10_C, 0x0C|tmp);
	tmp = SENSOR_IO_Read(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_MD1_CFG);
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_MD1_CFG, tmp|0x02);
}
void lsm6dsl_dready_en(void)
{
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_INT1_CTRL, 0x03);
}
void lsm6dsl_dready_dis(void)
{
	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_INT1_CTRL, 0x0);
}

void lis3mdl_dready_en(void)
{
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}
void lis3mdl_dready_dis(void)
{
	HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
}
void lps22hb_dready_en(void)
{
	SENSOR_IO_Write(LPS22HB_I2C_ADDRESS,LPS22HB_CTRL_REG3,0b100);
}
void lps22hb_dready_dis(void)
{
	SENSOR_IO_Write(LPS22HB_I2C_ADDRESS,LPS22HB_CTRL_REG3,0b0);
}
void hts221_dready_en(void)
{
	SENSOR_IO_Write(HTS221_I2C_ADDRESS,HTS221_CTRL_REG3,0b100);
}
void hts221_dready_dis(void)
{
	SENSOR_IO_Write(HTS221_I2C_ADDRESS,HTS221_CTRL_REG3,0b0);
}
//script of lps22hb error: drdy enable for default
void sensor_config(void)
{
	lsm6dsl_relative_tilt_intr_en();
//	lsm6dsl_single_tap_intr_en();
//	SENSOR_IO_Write(LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW, LSM6DSL_ACC_GYRO_DRDY_PULSE_CFG_G, 1<<7);
//	lsm6dsl_dready_en();
//	lis3mdl_dready_en();

//	lps22hb_dready_en();
//	hts221_dready_en();
}
//avoiding sprintf
int convert(float data,int integer[],int floating[])
{
	data = data/100;//recover data to what it is;
	int dataint = (int)data;
	float datafloat = data-dataint;
	int i;
	for(i=0;i<10;i++)
	{
		integer[i] = dataint%10;
		dataint/=10;
		if(dataint==0)break;
	}
	datafloat = (int)datafloat*10000;
	floating[1] = (int)datafloat%10000;
	floating[2] = (int)datafloat%1000;
	floating[3] = (int)datafloat%100;
	floating[4] = (int)datafloat%10;
	return i+1;
}
