/*
 * bmp180.h
 *
 *  Created on: 5 мар. 2020 г.
 *      Author: Dmitry
 */

#ifndef INC_BMP180_H_
#define INC_BMP180_H_

typedef enum bmp180_result_t
{
	BMP180_OK = 0,
	BMP180_ERROR = -1
} bmp180_result_t;

typedef enum bmp180_ercode_t
{
	BMP180_NONE = 0,
	BMP180_I2C_ERROR = -10
} bmp180_ercode_t;

typedef enum bmp180_precision_t
{
	BMP180_PRECISION_0 = 0,
	BMP180_PRECISION_1 = 1,
	BMP180_PRECISION_2 = 2,
	BMP180_PRECISION_3 = 3,
} bmp180_precision_t;


typedef struct bmp180_data_t
{
	int16_t temperature;
	int32_t pressure;
} bmp180_data_t;




// Send port and pin, which are connected to DHT22 module.
bmp180_result_t bmp180_init(I2C_HandleTypeDef* i2c);

void bmp180_set_precision(bmp180_precision_t precision);

// Reads data from DHT22.
bmp180_result_t bmp180_read(bmp180_data_t *data_out);

bmp180_ercode_t get_error_code();


#endif /* INC_BMP180_H_ */
