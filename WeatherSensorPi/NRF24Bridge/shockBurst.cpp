#include "shockBurst.h"

#include <cstdlib>
#include <iostream>
#include <stdint.h>
/*
 * Подключаем файл настроек из библиотеки RF24
 */
#include <RF24/nRF24L01.h>
 /*
  * Подключаем библиотеку  для работы с nRF24L01+
  */
#include <RF24/RF24.h>

using namespace std;

#define CE1 22
#define CSN1 0

#define CE2 27
#define CSN2 1

#define MODE_TX 10
#define MODE_RX 11

#define MAX_LEN 32

/*
 * Создаём массив для приёма данных
 */
uint8_t receivedData[MAX_LEN];
uint8_t pipeNumber;
uint8_t payloadSize;

/*
 * номера труб
 */
uint8_t pipesAddress[6][5] = {

{ '0', 'P', 'I', 'P', 'E' },

{ '1', 'P', 'I', 'P', 'E' },

{ '2', 'P', 'I', 'P', 'E' },

{ '3', 'P', 'I', 'P', 'E' },

{ '4', 'P', 'I', 'P', 'E' },

{ '5', 'P', 'I', 'P', 'E' },

};

RF24* init_rf(int ce, int csn, int mode)
{
	RF24* radio = new RF24(ce, csn);

	radio->begin();

	if (!radio->isChipConnected())
	{
		printf("Chip %d is not connected!\n", csn);
		delete radio;
		return nullptr;
	}
	radio->setAutoAck(1);
	radio->setRetries(0, 15);
	radio->enableDynamicPayloads();

	if (mode == MODE_RX)
	{
		for (int i = 0; i < 6; i++)
		{
			radio->openReadingPipe(i, pipesAddress[i]);
		}
	}
	else
	{
		radio->openWritingPipe(pipesAddress[0]);
		/*
		  Не слушаем радиоэфир, мы передатчик
		*/
		radio->stopListening();
	}
	
	radio->setChannel(0x05);
	radio->setPALevel(RF24_PA_MAX);
	radio->setDataRate(RF24_1MBPS);

	return radio;
}

void shockBurst(bool selfTransmitterEnabled, bool useTextFormat)
{
	RF24* transmitter = nullptr;
	if (selfTransmitterEnabled)
	{
		transmitter = init_rf(CE2, CSN2, MODE_TX);
		if (transmitter == nullptr)
			return;
		printf("Transmitter parameters: \n");
		transmitter->printDetails();
	}
	
	
	RF24* radio = init_rf(CE1, CSN1, MODE_RX);
	if (radio == nullptr)
	{
		if (transmitter != nullptr)
		{
			delete transmitter;
		}
		return;
	}
	
	radio->startListening();
	
	printf("Receiver parameters: \n");
	radio->printDetails();

	uint8_t payloadSize = 16;
	uint8_t* data = new uint8_t[payloadSize];
	
	cout << "startListening" << endl;
	while (true)
	{
		if (selfTransmitterEnabled)
		{
			for (uint8_t i = 0; i < payloadSize; i++)
			{
				data[i] = 'a' + i;
			}
			printf("Sending: ");
			for (uint8_t i = 0; i < payloadSize; i++)
			{
				printf("%02X ", data[i]);
			}
			printf("\n");
			if (transmitter->write(data, payloadSize))
			{
				printf("Data accepted by receiver.\n");
			}
			else {
				printf("Sending failed.\n");
			}
		}
		
		if (radio->available(&pipeNumber)) 
		{
			payloadSize = radio->getDynamicPayloadSize();
			/*
			 * Читаем данные в массив data и указываем сколько байт читать
			 */
			radio->read(&receivedData, payloadSize);
			cout << "Pipe: " << (int)pipeNumber << "; ";
			cout << "Size: ";
			printf("%02d", (int)payloadSize);
			cout << "; ";
			cout << "Data: [";
			if (!useTextFormat)
			{
				for (uint8_t i = 0; i < payloadSize; ++i) {
					if (i == 0) {
						printf("%02X", receivedData[i]);
					}
					else {
						printf(", %02X", receivedData[i]);
					}
				}
			}
			else
			{
				printf((const char*)receivedData);
			}
			cout << "]" << endl;
		}
		delay(500);
	}
}