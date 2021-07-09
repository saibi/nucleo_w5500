/*!
 * \file event_loop.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/17 15:36:34 (KST)
 *
*/
#include "event_loop.h"

#include <version.h>
#include <global_def.h>
#include <macro.h>
#include <wiz_appl.h>
#include <tester.h>
#include <comm_base.h>

int event_loop_delay = 0;

void set_event_loop_delay(int val)
{
	event_loop_delay = val;
	DPN("event_loop_delay = %d", event_loop_delay);
}

void event_loop(void)
{
	unsigned int prev_tick = 0;

	DPN("w5500 test v%s b%s %s %s", version_str, build_count_str, date_str, time_str);
	malloc_test();

	wiz_init_chip();

	DPN("start main loop");
	while (1)
	{
		comm_handler();

		if ( run_per_x_seconds(&prev_tick, 60, CURRENT_TICK, TICKS_PER_SECOND) )
		{
			DPN("");
		}
		HAL_Delay(event_loop_delay);
	}
}
/********** end of file **********/
