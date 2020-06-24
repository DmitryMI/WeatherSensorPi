/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.

 03/17/2013 : Charles-Henri Hallard (http://hallard.me)
			  Modified to use with Arduipi board http://hallard.me/arduipi
						  Changed to use modified bcm2835 and RF24 library
TMRh20 2014 - Updated to work with optimized RF24 Arduino library

 */

 /**
  * Example RF Radio Ping Pair
  *
  * This is an example of how to use the RF24 class on RPi, communicating to an Arduino running
  * the GettingStarted sketch.
  */

//#define BCM2835_NO_DELAY_COMPATIBILITY

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <chrono>
#include <ctime>
#include <syslog.h>

#include <RF24/RF24.h>

#include "daemonize.h"
#include "shockBurst.h"
#include "sserver.h"
#include "WeatherSensor.h"

#define POLL_PERIOD_SEC 10

using namespace std;

SServer* server;

void sensor_received(int pipe, uint8_t *data, int size)
{
    uint8_t* package = new uint8_t[size + 1];
    package[0] = (uint8_t)pipe;

	for(int i = 0; i < size; i++)
	{
        package[i + 1] = data[i];
	}
    server->sendDataToAll(package, size + 1);

    delete[] package;
}

int main()
{
    //shockBurst(false, false);
    //return 0;
	
    server = new SServer(3001);
    server->startServer();

    WeatherSensor* sensor = new WeatherSensor(sensor_received);

	printf("WeatherSensorPi started!\n");

#ifdef RUN_AS_DAEMON
	printf("Daemonizing...\n");
	daemonize("WeatherSensorPi");

	if (already_running() != 0)
	{
		syslog(LOG_ERR, "Daemon is already running\n");
		exit(1);
	}

	syslog(LOG_WARNING, "Daemon started sucessfully");
#else
	printf("Daemonizing skipped\n");
#endif

	while(true)
	{
		syslog(LOG_INFO, "Polling connections...\n");
        server->pollConnections();

		syslog(LOG_INFO, "Polling radio...\n");
        sensor->pollRadio();

        sleep(POLL_PERIOD_SEC);
	}

	getchar();
}