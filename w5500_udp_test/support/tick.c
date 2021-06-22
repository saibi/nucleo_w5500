/*!
 * \file tick.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/05/17 10:45:03 (KST)
 *
*/
#include "tick.h"

/// x 초에 한번 실행하기 위해
/// \param prev (in/out) 이전 tick 을 저장 변수 pointer
/// \param second 기준 초
/// \param cur_tick 현재 tick
/// \param tick_per_second 초당 tick 수
/// \return 1 x 초가 되는 시점이면 1 리턴
int run_per_x_seconds(unsigned int *prev, int second, unsigned int cur_tick, int tick_per_second)
{
	unsigned int cur;

	if ( second <= 0 ) {
		return 1;
	}

	if ( *prev == 0 ) {
		*prev = cur_tick;
		return 0; 
	} 
	cur = cur_tick;
	
	if ( *prev > cur ) {
		// rtc tick 이 overflow 되었을경우
		*prev = cur;
		return 1;
	}

	if ( (cur - (*prev) ) < tick_per_second * second) {
		return 0;
	}
	*prev = cur;
	return 1;
}


/// x tick 에 한번 실행하기 위해
/// \param prev (in/out) 이전 tick 을 저장 변수 pointer
/// \param x_ticks
/// \param cur_tick 현재 tick
/// \return 1 x 초가 되는 시점이면 1 리턴
int run_per_x_ticks(unsigned int *prev, int x_ticks, unsigned int cur_tick)
{
	unsigned int cur;

	if ( x_ticks <= 0 ) {
		return 1;
	}

	if ( *prev == 0 ) {
		*prev = cur_tick;
		return 0; 
	} 
	cur = cur_tick;
	
	if ( *prev > cur ) {
		// rtc tick 이 overflow 되었을경우
		*prev = cur;
		return 1;
	}

	if ( (cur - (*prev) ) < x_ticks) {
		return 0;
	}
	*prev = cur;
	return 1;
}

/********** end of file **********/
