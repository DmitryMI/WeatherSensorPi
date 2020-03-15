#pragma once
#include <cstdint>
#include <RF24/RF24.h>

#define CE1 22
#define CSN1 0

#define CE2 27
#define CSN2 1

#define MODE_TX 10
#define MODE_RX 11

typedef void sensor_callback_t(int pipeNumber, uint8_t *sensor_data, int count);

class WeatherSensor
{
public:
	WeatherSensor(sensor_callback_t* callback);
	~WeatherSensor();

	void pollRadio();
	void pollRadioMock();
private:
	sensor_callback_t* callback;
	RF24* radio = nullptr;
};

