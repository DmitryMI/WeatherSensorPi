#include "WeatherSensor.h"
#include <iostream>

/*
 * номера труб
 */
uint8_t sensor_pipesAddress[6][5] = {

{ '0', 'P', 'I', 'P', 'E' },

{ '1', 'P', 'I', 'P', 'E' },

{ '2', 'P', 'I', 'P', 'E' },

{ '3', 'P', 'I', 'P', 'E' },

{ '4', 'P', 'I', 'P', 'E' },

{ '5', 'P', 'I', 'P', 'E' },

};

void weather_init_rf(RF24* radio, uint16_t ce, uint16_t csn, int mode)
{

	radio->begin();

	if (!radio->isChipConnected())
	{
		printf("Chip %d is not connected!\n", csn);
	}
	radio->setAutoAck(1);
	radio->setRetries(0, 15);
	radio->enableDynamicPayloads();

	if (mode == MODE_RX)
	{
		for (int i = 0; i < 6; i++)
		{
			radio->openReadingPipe((uint8_t)i, sensor_pipesAddress[i]);
		}
	}
	else
	{
		radio->openWritingPipe(sensor_pipesAddress[0]);
		/*
		  Не слушаем радиоэфир, мы передатчик
		*/
		radio->stopListening();
	}

	radio->setChannel(0x05);
	radio->setPALevel(RF24_PA_MAX);
	radio->setDataRate(RF24_1MBPS);

}

WeatherSensor::WeatherSensor(sensor_callback_t* callback)
{
	this->callback = callback;

	radio = new RF24(CE1, CSN1);
	weather_init_rf(radio,CE1, CSN1, MODE_RX);
	if (radio == nullptr)
	{
		return;
	}

	radio->startListening();

	printf("Receiver parameters: \n");
	radio->printDetails();

	std::cout << "startListening" << std::endl;
}

void WeatherSensor::pollRadio()
{
	uint8_t pipeNumber = 0;
	if (radio->available(&pipeNumber))
	{
		uint8_t payloadSize = radio->getDynamicPayloadSize();
		uint8_t receivedData[8] = {0};

		if(payloadSize > sizeof receivedData)
		{
			printf("Received data size is bigger than buffer size\n");
		}

		radio->read(&receivedData, payloadSize);
		
		printf("Pipe: %02d, payload size: %02d\n", pipeNumber, payloadSize);

		callback(pipeNumber, receivedData, payloadSize);
	}
}

void WeatherSensor::pollRadioMock()
{
	uint8_t example[] = { 250, 0, 88, 2, 136, 138, 1, 0};
	callback(0, example, sizeof example);
}
