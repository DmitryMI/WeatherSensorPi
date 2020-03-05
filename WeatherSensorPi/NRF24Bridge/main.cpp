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

#include <RF24/RF24.h>

#include "shockBurst.h"

using namespace std;

int main()
{
	printf("Shit happens :)\n");
	
	shockBurst(false, true);

	getchar();
}