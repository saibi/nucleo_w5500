/*!
 * \file tick.h
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/03/18 13:32:03 (KST)
 *
*/
#ifndef _TICK_H_
#define _TICK_H_

#include "stm_dep.h"

#define CURRENT_TICK ((unsigned int)uwTick)
#define TICKS_PER_SECOND (1000)


/// rtc 를 이용한 time loop macro
/// loop seconds 동안 반복 (초단위)
#define TIME_LOOP_START(loop_seconds) \
	{ \
		unsigned int tl_cur_time = CURRENT_TICK; \
		unsigned int end_tick = tl_cur_time + (loop_seconds) * TICKS_PER_SECOND; \
		while ( (CURRENT_TICK) < end_tick ) {

#define TIME_LOOP_END }}



int run_per_x_seconds(unsigned int *prev, int second, unsigned int cur_tick, int tick_per_second);
int run_per_x_ticks(unsigned int *prev, int x_ticks, unsigned int cur_tick);


#endif 
/********** end of file **********/
