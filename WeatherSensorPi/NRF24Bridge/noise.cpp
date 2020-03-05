
#include "noise.h"
#include <iostream>

using namespace std;

#define PIN_CE 15
#define PIN_CSN 8
#define NUM_CHANNELS 126

static uint8_t values[NUM_CHANNELS];
static const int num_reps = 100;

int channel_noise(RF24 *radio, int count)
{	
	radio->begin();
	radio->setAutoAck(false); // запрет на автоматическую отправку пакетов подтверждени€ приема дл€ всех труб

	radio->startListening(); // вход в режиме приемника
	radio->stopListening();

	radio->printDetails(); // вывод конфигурации

	// ¬ывод верхнего заголовка
	for (int i = 0; i < NUM_CHANNELS; ++i)
	{
		cout << hex << (i >> 4);
	}

	cout << endl;

	// ¬ывод нижнего заголовка
	for (int i = 0; i < NUM_CHANNELS; ++i)
	{
		cout << hex << (i & 0xf);
	}

	cout << endl;

	for(int c = 0; c < count; c++)
	{
		memset(values, 0, sizeof(values)); // все значени€ каналов обнул€ем

		// —канирование всех каналов num_reps
		for (int k = 0; k < num_reps; ++k)
		{
			for (int i = 0; i < NUM_CHANNELS; ++i)
			{

				radio->setChannel((uint8_t)i);

				radio->startListening();
				delayMicroseconds(130); // т.к. врем€ переключени€ между модул€ каналами состовл€ет 130 мкс
				radio->stopListening();

				// ѕроверка наличи€ несущей частоты на выбранном канале (частоте)
				if (radio->testCarrier())
				{
					++values[i];
				}
			}
		}

		// –аспечатка измерени€ канала в одну шестнадцатеричную цифру
		for (int i = 0; i < NUM_CHANNELS; ++i)
		{
			cout << hex << min(0xf, (values[i] & 0xf));
		}
		cout << endl;
	}

	return 0;
}