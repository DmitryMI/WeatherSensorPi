/*
 * dwt_timer.c
 *
 *  Created on: Mar 4, 2020
 *      Author: Dmitry
 */


#include "main.h"
#include "delay.h"

static TIM_HandleTypeDef *delay_htim;


void delay_init(TIM_HandleTypeDef *htim)
{
	delay_htim = htim;

	HAL_TIM_Base_Start(htim);
}

void delay_us(uint16_t us) // DelayMicro
{
	__HAL_TIM_SET_COUNTER(delay_htim,0);  // set the counter value a 0
	while (1)
	{
		uint32_t cnt = __HAL_TIM_GET_COUNTER(delay_htim);
		uint32_t us32 = (uint32_t)us;
		int isBigger = cnt >= us32;
		if(isBigger)
		{
			break;
		}
	}
}

void delay_ms(uint16_t ms)
{
	for(int i = 0; i < ms; i++)
	{
		delay_us(ms);
	}
}

void delay_start()
{
	__HAL_TIM_SET_COUNTER(delay_htim,0);  // set the counter value a 0
}

uint32_t delay_stop()
{
	return __HAL_TIM_GET_COUNTER(delay_htim);
}
