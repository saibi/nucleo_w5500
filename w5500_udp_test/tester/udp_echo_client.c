/*!
 * \file udp_echo_client.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/05/17 11:44:31 (KST)
 *
*/
#include "tester.h"
#include "helper.h"

#include <string.h>

#include <macro.h>

#include <socket.h>

int udp_echo_test(uint8_t dest_addr[4], int dest_port, char * send_str)
{
#define SOCK_NO 6

	int ret = 0;
	int src_port = gen_random_port();
	struct ip_port_rec dest;

	SET_IP_PORT(dest, dest_addr, dest_port);

	DPN("udp echo test");

	DPN("src port = %d", src_port);

	DP("prepare udp echo server : ");
	prn_ip_port(&dest);

	ret = socket(SOCK_NO, Sn_MR_UDP, src_port, 0);
	DPN("create udp socket, ret = %d", ret);

	if ( ret != SOCK_NO )
	{
		DPN("socket error");
		return -1;
	}

	int send_bytes = strlen(send_str);
	ret = sendto(SOCK_NO, (uint8_t *)send_str, send_bytes, dest.ip, dest.port);

	DPN("sendto %d bytes, ret = %d", send_bytes, ret);


        HAL_Delay(100);

	uint8_t addr[4] = { 0, };
	uint16_t port = 0 ;
	char recv_buf[128] = {0, };

	ret = recvfrom(SOCK_NO, (uint8_t*)recv_buf, sizeof(recv_buf), addr, &port);
	DPN("recvfrom, ret = %d", ret);

	if ( ret > 0 ) 
	{
		DP("recv [%s] from ", recv_buf);
		prn_ip_port(&dest);
	}

	ret = close(SOCK_NO);
	DPN("close ret = %d", ret);

	return 0;
}
/********** end of file **********/
