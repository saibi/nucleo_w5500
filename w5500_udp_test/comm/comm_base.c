/*!
 * \file comm_base.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/25 15:15:04 (KST)
 *
*/
#include "comm_base.h"
#include "comm_udp.h"
#include "comm_tcp3.h"
#include <global_def.h>
#include <macro.h>
#include <wiz_appl.h>
#include <tester.h>

enum socket_idx 
{
	SOCKET_NO_TCP_8K = 0,
	SOCKET_NO_UDP,
};

struct comm_meta_rec comm;

static wiz_NetInfo netinfo = {
	.mac = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0x44},
	.ip = {172, 10, 150, 131},
	.sn = {255, 255, 0, 0},
	.gw = {172, 10, 255, 254},
	.dns = {172, 16, 4, 3},
	.dhcp = NETINFO_STATIC,
};

void comm_handler(void)
{
	switch( comm.operation ) 
	{
	case OP_DHCP:
		wiz_set_dhcp_ip(&netinfo);
		comm.operation = OP_WAIT;
		break;

	default:
		comm_udp_server(SOCKET_NO_UDP);

		if ( comm_tcp_client(SOCKET_NO_TCP_8K) )
			comm_tcp_packet_handler(SOCKET_NO_TCP_8K);

		break;
	}
}
/********** end of file **********/
