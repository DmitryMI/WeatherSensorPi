/*
 * dwt_timer.h
 *
 *  Created on: Mar 4, 2020
 *      Author: Dmitry
 */

#ifndef INC_DWT_TIMER_H_
#define INC_DWT_TIMER_H_

#include <inttypes.h>

void delay_init(TIM_HandleTypeDef *htim);

void delay_us(uint16_t us);

void delay_ms(uint16_t ms);

void delay_start();

uint32_t delay_stop();

#endif /* INC_DWT_TIMER_H_ */
