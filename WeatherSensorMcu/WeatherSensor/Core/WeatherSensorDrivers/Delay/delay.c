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

void delay_us(uint32_t us) // DelayMicro
{
	__HAL_TIM_SET_COUNTER(delay_htim,0);  // set the counter value a 0
	while (__HAL_TIM_GET_COUNTER(delay_htim) < us);
}

void delay_start()
{
	__HAL_TIM_SET_COUNTER(delay_htim,0);  // set the counter value a 0
}

uint32_t delay_stop()
{
	return __HAL_TIM_GET_COUNTER(delay_htim);
}
