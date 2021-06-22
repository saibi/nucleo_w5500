/*!
 * \file tcp_echo_client.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/05/17 10:34:25 (KST)
 *
*/
#include "tester.h"
#include "helper.h"

#include <string.h>
#include <macro.h>
#include <socket.h>


/// tcp echo client
/// \param dest_addr
/// \param dest_port
/// \return 0 success
int tcp_echo_test(void)
{
	static int counter = 0;
	uint8_t dest_addr[4] = { 172, 10, 150, 130 };
	uint16_t dest_port = 1234;

#define SOCK_NO 7
	int ret = 0;
	int src_port = gen_random_port();

	DPN("tcp echo test");
	
	ret = socket(SOCK_NO, Sn_MR_TCP, src_port, 0);
	DPN("create socket, port = %d, ret = %d", src_port, ret);

	DPN("prepare tcp echo server : %d.%d.%d.%d:%d", 
			dest_addr[0], 
			dest_addr[1], 
			dest_addr[2], 
			dest_addr[3], 
			dest_port);
			
	ret = connect(SOCK_NO, dest_addr, dest_port);

	DPN("connect ret = %d", ret);

	char send_buf[128] = { 0, };

	snprintf(send_buf, sizeof(send_buf), "Hello World !!! #%d", ++counter);

	int bytes_to_send = strlen(send_buf);

	ret = send(SOCK_NO, (uint8_t *)send_buf, bytes_to_send);

	DPN("send %d bytes", ret);


	char recv_buf[128] = { 0, };

	ret = recv(SOCK_NO, (uint8_t *)recv_buf, sizeof(recv_buf) -1 );

	DPN("recv %d bytes", ret);
	if ( ret > 0 ) 
		DPN("[%s]", recv_buf);

	close(SOCK_NO);

	if ( strncmp(send_buf, recv_buf, bytes_to_send) == 0 )
	{
		DPN("tcp echo test OK");
		return 0;
	}
	return -1;
}
/********** end of file **********/
