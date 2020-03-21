/*
 * am2320.c
 *
 *  Created on: Mar 5, 2020
 *      Author: Dmitry
 */

#include "am2320.h"

typedef struct
{
	I2C_HandleTypeDef* i2c_handle;
	uint8_t device_address;
	uint8_t data[8];
} am2320_HandleTypeDef;

static am2320_HandleTypeDef am2320_handle;

void am2320_init(I2C_HandleTypeDef* i2c)
{
	am2320_handle.i2c_handle = i2c;
	am2320_handle.device_address = 0x5C << 1;
}

uint8_t am2320_readValue1(am2320_HandleTypeDef *am2320)
{
	uint8_t registers[3] = { 0x03, 0x00, 0x04 };
	HAL_I2C_Master_Transmit(am2320->i2c_handle, am2320->device_address, 0x00, 0,
	1000);
	HAL_Delay(1);
	if (HAL_I2C_Master_Transmit(am2320->i2c_handle, am2320->device_address,
			registers, 3, 1000) != HAL_OK)
	{
		return 1;
	}

	HAL_Delay(2);
	if (HAL_I2C_Master_Receive(am2320->i2c_handle, am2320->device_address,
			am2320->data, 8, 1000) != HAL_OK)
	{
		return 2;
	}

	if (am2320->data[1] != 0x04 && am2320->data[0] != 0x03)
	{
		return 3;
	}
	//TODO 04.08.2019 add CRC calculation
	return 0;
}

#if AM2320_FORMAT_USE_FLOAT

int am2320_getValue(am2320_HandleTypeDef *am2320, float *temperature,
		float *humidity)
{
	int read = am2320_readValue1(am2320);
	if (read != 0)
	{
		return read;
	}
	uint16_t temp_temperature = (am2320->data[5] | am2320->data[4] << 8);
	if (temp_temperature & 0x8000) {
		temp_temperature = -(int16_t) (temp_temperature & 0x7fff);
	} else {
		temp_temperature = (int16_t) temp_temperature;
	}
	*temperature = (float) temp_temperature / 10.0;
	*humidity = (float) (am2320->data[3] | am2320->data[2] << 8) / 10.0;
	return 0;
}

#else
int am2320_getValue(am2320_HandleTypeDef *am2320, uint16_t *temperature,
		uint16_t *humidity)
{
	int read = am2320_readValue1(am2320);
	if (read != 0)
	{
		return read;
	}
	uint16_t temp_temperature = (am2320->data[5] | am2320->data[4] << 8);
	if (temp_temperature & 0x8000) {
		temp_temperature = -(int16_t) (temp_temperature & 0x7fff);
	} else {
		temp_temperature = (int16_t) temp_temperature;
	}
	*temperature = temp_temperature;
	*humidity = (am2320->data[3] | am2320->data[2] << 8);
	return 0;
}
#endif

am2320_result_t am2320_read(am2320_data_t *data_out)
{
	uint16_t temp, humidity;
	int result = am2320_getValue(&am2320_handle, &temp, &humidity);

	data_out->temperature = temp;
	data_out->humidity = humidity;

	if(result != 0)
	{
		return AM2320_ERROR;
	}
	return AM2320_OK;
}



