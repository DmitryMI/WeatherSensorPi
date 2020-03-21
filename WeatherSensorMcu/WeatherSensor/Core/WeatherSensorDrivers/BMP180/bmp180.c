/*
 * bmp180.c
 *
 *  Created on: 5 мар. 2020 г.
 *      Author: Dmitry
 */

#include "main.h"
#include "bmp180.h"

/* BMP180 defines */
#define BMP180_ADDR                     0xEE // BMP180 address
/* BMP180 registers */
#define BMP180_PROM_START_ADDR          0xAA // E2PROM calibration data start register
#define BMP180_PROM_DATA_LEN            22   // E2PROM length
#define BMP180_CHIP_ID_REG              0xD0 // Chip ID
#define BMP180_VERSION_REG              0xD1 // Version
#define BMP180_CTRL_MEAS_REG            0xF4 // Measurements control (OSS[7.6], SCO[5], CTL[4.0]
#define BMP180_ADC_OUT_MSB_REG          0xF6 // ADC out MSB  [7:0]
#define BMP180_ADC_OUT_LSB_REG          0xF7 // ADC out LSB  [7:0]
#define BMP180_ADC_OUT_XLSB_REG         0xF8 // ADC out XLSB [7:3]
#define BMP180_SOFT_RESET_REG           0xE0 // Soft reset control
/* BMP180 control values */
#define BMP180_T_MEASURE                0x2E // temperature measurement
#define BMP180_P0_MEASURE               0x34 // pressure measurement (OSS=0, 4.5ms)
#define BMP180_P1_MEASURE               0x74 // pressure measurement (OSS=1, 7.5ms)
#define BMP180_P2_MEASURE               0xB4 // pressure measurement (OSS=2, 13.5ms)
#define BMP180_P3_MEASURE               0xF4 // pressure measurement (OSS=3, 25.5ms)
/* BMP180 Pressure calculation constants */
#define BMP180_PARAM_MG                 3038
#define BMP180_PARAM_MH                -7357
#define BMP180_PARAM_MI                 3791


/* Calibration parameters structure */
typedef struct
{
	int16_t AC1;
	int16_t AC2;
	int16_t AC3;
	uint16_t AC4;
	uint16_t AC5;
	uint16_t AC6;
	int16_t B1;
	int16_t B2;
	int16_t MB;
	int16_t MC;
	int16_t MD;
	int32_t B5;
} BMP180_Calibration_TypeDef;


/* Calibration parameters from E2PROM of BMP180 */
static BMP180_Calibration_TypeDef BMP180_Calibration;
static I2C_HandleTypeDef* hi2c;
static bmp180_ercode_t error_code;
static bmp180_precision_t precision;


void BMP180_ReadCalibration(void)
{
	uint8_t buffer[BMP180_PROM_DATA_LEN];

	/*I2C_AcknowledgeConfig(I2C_PORT,ENABLE); // Enable I2C acknowledge
	I2C_GenerateSTART(I2C_PORT,ENABLE); // Send START condition
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_MODE_SELECT)); // Wait for EV5
	I2C_Send7bitAddress(I2C_PORT,BMP180_ADDR,I2C_Direction_Transmitter); // Send slave address
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)); // Wait for EV6
	I2C_SendData(I2C_PORT,BMP180_PROM_START_ADDR); // Send calibration first register address
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_BYTE_TRANSMITTED)); // Wait for EV8
	I2C_GenerateSTART(I2C_PORT,ENABLE); // Send repeated START condition (aka Re-START)
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_MODE_SELECT)); // Wait for EV5
	I2C_Send7bitAddress(I2C_PORT,BMP180_ADDR,I2C_Direction_Receiver); // Send slave address for READ
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)); // Wait for EV6
	for (i = 0; i < BMP180_PROM_DATA_LEN-1; i++) {
		while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_BYTE_RECEIVED)); // Wait for EV7 (Byte received from slave)
		buffer[i] = I2C_ReceiveData(I2C_PORT); // Receive byte
	}
	I2C_AcknowledgeConfig(I2C_PORT,DISABLE); // Disable I2C acknowledgment
	I2C_GenerateSTOP(I2C_PORT,ENABLE); // Send STOP condition
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_BYTE_RECEIVED)); // Wait for EV7 (Byte received from slave)
	buffer[i] = I2C_ReceiveData(I2C_PORT); // Receive last byte
	*/

	if(HAL_I2C_Mem_Read(hi2c, BMP180_ADDR, BMP180_PROM_START_ADDR, 1, buffer, BMP180_PROM_DATA_LEN, 1000) != HAL_OK)
	{
		error_code = BMP180_I2C_ERROR;
	}


	BMP180_Calibration.AC1 = (buffer[0]  << 8) | buffer[1];
	BMP180_Calibration.AC2 = (buffer[2]  << 8) | buffer[3];
	BMP180_Calibration.AC3 = (buffer[4]  << 8) | buffer[5];
	BMP180_Calibration.AC4 = (buffer[6]  << 8) | buffer[7];
	BMP180_Calibration.AC5 = (buffer[8]  << 8) | buffer[9];
	BMP180_Calibration.AC6 = (buffer[10] << 8) | buffer[11];
	BMP180_Calibration.B1  = (buffer[12] << 8) | buffer[13];
	BMP180_Calibration.B2  = (buffer[14] << 8) | buffer[15];
	BMP180_Calibration.MB  = (buffer[16] << 8) | buffer[17];
	BMP180_Calibration.MC  = (buffer[18] << 8) | buffer[19];
	BMP180_Calibration.MD  = (buffer[20] << 8) | buffer[21];
}

uint8_t BMP180_ReadReg(uint8_t reg)
{
	uint8_t value;

	/*I2C_GenerateSTART(I2C_PORT,ENABLE);
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_MODE_SELECT)); // Wait for EV5
	I2C_Send7bitAddress(I2C_PORT,BMP180_ADDR,I2C_Direction_Transmitter); // Send slave address
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)); // Wait for EV6
	I2C_SendData(I2C_PORT,reg); // Send register address
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_BYTE_TRANSMITTED)); // Wait for EV8
	I2C_GenerateSTART(I2C_PORT,ENABLE); // Send repeated START condition (aka Re-START)
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_MODE_SELECT)); // Wait for EV5
	I2C_Send7bitAddress(I2C_PORT,BMP180_ADDR,I2C_Direction_Receiver); // Send slave address for READ
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)); // Wait for EV6
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_BYTE_RECEIVED)); // Wait for EV7 (Byte received from slave)
	value = I2C_ReceiveData(I2C_PORT); // Receive ChipID
	I2C_AcknowledgeConfig(I2C_PORT,DISABLE); // Disable I2C acknowledgment
	I2C_GenerateSTOP(I2C_PORT,ENABLE); // Send STOP condition
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_BYTE_RECEIVED)); // Wait for EV7 (Byte received from slave)
	 */
	if(HAL_I2C_Mem_Read(hi2c, BMP180_ADDR, reg, 1, &value, 1, 1000) != HAL_OK)
	{
		error_code = BMP180_I2C_ERROR;
	}
	return value;
}

uint8_t BMP180_WriteReg(uint8_t reg, uint8_t value)
{
	/*I2C_GenerateSTART(I2C_PORT,ENABLE);
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_MODE_SELECT)); // Wait for EV5
	I2C_Send7bitAddress(I2C_PORT,BMP180_ADDR,I2C_Direction_Transmitter); // Send slave address
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)); // Wait for EV6
	I2C_SendData(I2C_PORT,reg); // Send register address
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_BYTE_TRANSMITTED)); // Wait for EV8
	I2C_SendData(I2C_PORT,value); // Send register value
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_BYTE_TRANSMITTED)); // Wait for EV8
	I2C_GenerateSTOP(I2C_PORT,ENABLE);*/
	uint8_t arr[2] = {reg, value};
	if(HAL_I2C_Master_Transmit(hi2c, BMP180_ADDR, arr, sizeof(arr), 1000) != HAL_OK)
	{
		error_code = BMP180_I2C_ERROR;
	}

	return value;
}

void BMP180_Reset()
{
	BMP180_WriteReg(BMP180_SOFT_RESET_REG,0xb6); // Do software reset
}

uint16_t BMP180_Read_UT(void)
{
	uint16_t UT;

	BMP180_WriteReg(BMP180_CTRL_MEAS_REG, BMP180_T_MEASURE);
	//Delay_ms(6); // Wait for 4.5ms by datasheet
	HAL_Delay(6);

	/*I2C_AcknowledgeConfig(I2C_PORT,ENABLE); // Enable I2C acknowledge
	I2C_GenerateSTART(I2C_PORT,ENABLE); // Send START condition
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_MODE_SELECT)); // Wait for EV5
	I2C_Send7bitAddress(I2C_PORT,BMP180_ADDR,I2C_Direction_Transmitter); // Send slave address
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)); // Wait for EV6
	I2C_SendData(I2C_PORT,BMP180_ADC_OUT_MSB_REG); // Send ADC MSB register address
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_BYTE_TRANSMITTED)); // Wait for EV8
	I2C_GenerateSTART(I2C_PORT,ENABLE); // Send repeated START condition (aka Re-START)
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_MODE_SELECT)); // Wait for EV5
	I2C_Send7bitAddress(I2C_PORT,BMP180_ADDR,I2C_Direction_Receiver); // Send slave address for READ
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)); // Wait for EV6
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_BYTE_RECEIVED)); // Wait for EV7 (Byte received from slave)
	UT = (uint16_t)I2C_ReceiveData(I2C_PORT) << 8; // Receive MSB
	I2C_AcknowledgeConfig(I2C_PORT,DISABLE); // Disable I2C acknowledgment
	I2C_GenerateSTOP(I2C_PORT,ENABLE); // Send STOP condition
	while (!I2C_CheckEvent(I2C_PORT,I2C_EVENT_MASTER_BYTE_RECEIVED)); // Wait for EV7 (Byte received from slave)
	UT |= I2C_ReceiveData(I2C_PORT); // Receive LSB*/

	uint8_t arr[2] = {0, 0};

	if(HAL_I2C_Mem_Read(hi2c, BMP180_ADDR, BMP180_ADC_OUT_MSB_REG, 1, arr, 2, 1000) != HAL_OK)
	{
		error_code = BMP180_I2C_ERROR;
	}

	UT = (arr[0] << 8) + arr[1];

	return UT;
}

uint32_t BMP180_Read_UP(uint8_t oss)
{
	uint32_t PT;
	uint8_t cmd,delay;

	switch(oss)
	{
	case 0:
		cmd = BMP180_P0_MEASURE;
		delay   = 6;
		break;
	case 1:
		cmd = BMP180_P1_MEASURE;
		delay   = 9;
		break;
	case 2:
		cmd = BMP180_P2_MEASURE;
		delay   = 15;
		break;
	case 3:
		cmd = BMP180_P3_MEASURE;
		delay   = 27;
		break;
	}

	BMP180_WriteReg(BMP180_CTRL_MEAS_REG,cmd);
	HAL_Delay(delay);

	uint8_t arr[3] = {0};

	if(HAL_I2C_Mem_Read(hi2c, BMP180_ADDR, BMP180_ADC_OUT_MSB_REG, 1, arr, 3, 1000) != HAL_OK)
	{
		error_code = BMP180_I2C_ERROR;
	}

	PT = (arr[0] << 16) + (arr[1] << 8) + arr[2];

	return PT >> (8 - oss);
}

int16_t BMP180_Calc_RT(uint16_t UT)
{
	BMP180_Calibration.B5  = (((int32_t)UT - (int32_t)BMP180_Calibration.AC6) * (int32_t)BMP180_Calibration.AC5) >> 15;
	BMP180_Calibration.B5 += ((int32_t)BMP180_Calibration.MC << 11) / (BMP180_Calibration.B5 + BMP180_Calibration.MD);

	return (BMP180_Calibration.B5 + 8) >> 4;
}

int32_t BMP180_Calc_RP(uint32_t UP, uint8_t oss)
{
	int32_t B3,B6,X3,p;
	uint32_t B4,B7;

	B6 = BMP180_Calibration.B5 - 4000;
	X3 = ((BMP180_Calibration.B2 * ((B6 * B6) >> 12)) >> 11) + ((BMP180_Calibration.AC2 * B6) >> 11);
	B3 = (((((int32_t)BMP180_Calibration.AC1) * 4 + X3) << oss) + 2) >> 2;
	X3 = (((BMP180_Calibration.AC3 * B6) >> 13) + ((BMP180_Calibration.B1 * ((B6 * B6) >> 12)) >> 16) + 2) >> 2;
	B4 = (BMP180_Calibration.AC4 * (uint32_t)(X3 + 32768)) >> 15;
	B7 = ((uint32_t)UP - B3) * (50000 >> oss);
	if (B7 < 0x80000000) p = (B7 << 1) / B4; else p = (B7 / B4) << 1;
	p += ((((p >> 8) * (p >> 8) * BMP180_PARAM_MG) >> 16) + ((BMP180_PARAM_MH * p) >> 16) + BMP180_PARAM_MI) >> 4;

	return p;
}


// Send port and pin, which are connected to DHT22 module.
bmp180_result_t bmp180_init(I2C_HandleTypeDef* i2c)
{
	hi2c = i2c;
	error_code = BMP180_NONE;
	precision = BMP180_PRECISION_3;

	BMP180_ReadCalibration();
	if(error_code != BMP180_NONE)
	{
		return BMP180_ERROR;
	}
	return BMP180_OK;
}

int bmp180_isChipConnected()
{
	uint8_t ChipID = BMP180_ReadReg(BMP180_CHIP_ID_REG);
	return ChipID == 0x55;
}

void bmp180_set_precision(bmp180_precision_t p)
{
	precision = p;
}

// Reads data from DHT22.
bmp180_result_t bmp180_read(bmp180_data_t *data_out)
{
	error_code = BMP180_NONE;

	uint16_t ut = BMP180_Read_UT();
	if(error_code != BMP180_NONE)
	{
		return BMP180_ERROR;
	}

	uint32_t up = BMP180_Read_UP(precision);
	if(error_code != BMP180_NONE)
	{
		return BMP180_ERROR;
	}

	int16_t rt = BMP180_Calc_RT(ut);
	int32_t rp = BMP180_Calc_RP(up, precision);

	if(error_code != BMP180_NONE)
	{
		return BMP180_ERROR;
	}

	data_out->temperature = rt;
	data_out->pressure = rp;

	return BMP180_OK;
}

bmp180_ercode_t get_error_code()
{
	return error_code;
}
