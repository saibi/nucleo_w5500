/*!
 * \file udp_echo_server.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/05/17 14:10:38 (KST)
 *
*/
#include "tester.h"
#include "helper.h"

#include <macro.h>

#include <socket.h>

int udp_echo_server_loop(int loop_seconds)
{
	int ret = 0;
	int recv_port = 1818;

#define SOCK_NO 5

	DPN("udp echo server (%d seconds)", loop_seconds);

	ret = socket(SOCK_NO, Sn_MR_UDP, recv_port, SOCK_IO_NONBLOCK);
	DPN("create udp socket, port %d, ret = %d", recv_port, ret);

	if ( ret != SOCK_NO )
	{
		DPN("socket error");
		return -1;
	}

	struct ip_port_rec sender;
	char recv_buf[2048] = {0, };

	DPN("start loop\n");

	TIME_LOOP_START(loop_seconds)

		ret = recvfrom(SOCK_NO, (uint8_t*)recv_buf, sizeof(recv_buf), sender.ip, &sender.port);
		if ( ret > 0 ) 
		{
			DP("recv %d bytes from ", ret);
			prn_ip_port(&sender);

			DPN("recv [%s]", recv_buf);

			ret = sendto(SOCK_NO, (uint8_t*)recv_buf, ret, sender.ip, sender.port);
			DPN("sendto, ret = %d", ret);
		}

        	HAL_Delay(100);

	TIME_LOOP_END

	DPN("end loop\n");

	ret = close(SOCK_NO);
	DPN("close ret = %d", ret);

	return 0;
}


/********** end of file **********/
