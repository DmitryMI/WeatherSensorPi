
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
	radio->setAutoAck(false); // ������ �� �������������� �������� ������� ������������� ������ ��� ���� ����

	radio->startListening(); // ���� � ������ ���������
	radio->stopListening();

	radio->printDetails(); // ����� ������������

	// ����� �������� ���������
	for (int i = 0; i < NUM_CHANNELS; ++i)
	{
		cout << hex << (i >> 4);
	}

	cout << endl;

	// ����� ������� ���������
	for (int i = 0; i < NUM_CHANNELS; ++i)
	{
		cout << hex << (i & 0xf);
	}

	cout << endl;

	for(int c = 0; c < count; c++)
	{
		memset(values, 0, sizeof(values)); // ��� �������� ������� ��������

		// ������������ ���� ������� num_reps
		for (int k = 0; k < num_reps; ++k)
		{
			for (int i = 0; i < NUM_CHANNELS; ++i)
			{

				radio->setChannel((uint8_t)i);

				radio->startListening();
				delayMicroseconds(130); // �.�. ����� ������������ ����� ������ �������� ���������� 130 ���
				radio->stopListening();

				// �������� ������� ������� ������� �� ��������� ������ (�������)
				if (radio->testCarrier())
				{
					++values[i];
				}
			}
		}

		// ���������� ��������� ������ � ���� ����������������� �����
		for (int i = 0; i < NUM_CHANNELS; ++i)
		{
			cout << hex << min(0xf, (values[i] & 0xf));
		}
		cout << endl;
	}

	return 0;
}