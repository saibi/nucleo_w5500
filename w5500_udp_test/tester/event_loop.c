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
#include <comm_tcp3.h>

enum socket_idx 
{
	SOCKET_NO_TCP_8K = 0,
	SOCKET_NO_UDP,
};

int event_loop_delay = 0;

void set_event_loop_delay(int val)
{
	event_loop_delay = val;
	DPN("event_loop_delay = %d", event_loop_delay);
}

void event_loop(void)
{
	unsigned int prev_tick = 0;

	wiz_NetInfo netinfo = {
		.mac = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0x44},
		.ip = {172, 10, 150, 131},
		.sn = {255, 255, 0, 0},
		.gw = {172, 10, 255, 254},
		.dns = {172, 16, 4, 3},
		.dhcp = NETINFO_STATIC,
	};

	DPN("w5500 test v%s b%s %s %s", version_str, build_count_str, date_str, time_str);
	malloc_test();

	wiz_init_chip();
	wiz_set_dhcp_ip(&netinfo);

	DPN("start main loop");
	while (1)
	{
		comm_udp_server(SOCKET_NO_UDP);
		if ( comm_tcp_client(SOCKET_NO_TCP_8K) )
			comm_tcp_packet_handler(SOCKET_NO_TCP_8K);

		if ( run_per_x_seconds(&prev_tick, 60, CURRENT_TICK, TICKS_PER_SECOND) )
		{
			DPN("");
		}
		HAL_Delay(event_loop_delay);
	}
}
/********** end of file **********/
