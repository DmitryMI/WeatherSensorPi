/*
 * am2320.h
 *
 *  Created on: Mar 5, 2020
 *      Author: Dmitry
 */

#ifndef INC_AM2320_H_
#define INC_AM2320_H_

#define AM2320_FORMAT_USE_FLOAT 0


#include "main.h"
#include <inttypes.h>

typedef enum am2320_result_t
{
	AM2320_OK = 0,
	AM2320_ERROR,
	AM2320_PARITY_FAIL
} am2320_result_t;

#if AM2320_FORMAT_USE_FLOAT

typedef struct am2320_data_t
{
	float temperature;
	float humidity;
} am2320_data_t;

#else
typedef struct am2320_data_t
{
	uint16_t temperature;
	uint16_t humidity;
} am2320_data_t;
#endif



// Send port and pin, whic are connected to DHT22 module.
// Be sure, that any function is invoked not earlier than 1 second after power-up
void am2320_init(I2C_HandleTypeDef* i2c);

// Reads data from DHT22.
// Be sure, that any function is invoked not earlier than 1 second after power-up
am2320_result_t am2320_read(am2320_data_t *data_out);


#endif /* INC_AM2320_H_ */
