/*!
 * \file comm_tcp.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/30 13:58:00 (KST)
 *
*/

#include "comm_tcp.h"
#include "comm_base.h"
#include <macro.h>
#include <wiz_appl.h>
#include <socket.h>

extern struct comm_meta_rec comm;

struct tcp_comm_meta_rec 
{
	char header_buf[TCP_HEADER_SIZE+4];
	int received_size;
} tcp_comm;


static int wait_tcp_header(int sock)
{
	int ret; 

	if ( tcp_comm.received_size < TCP_HEADER_SIZE ) 
	{
		int recv_size = TCP_HEADER_SIZE - tcp_comm.received_size;

		ret = recv(sock, (uint8_t*)&tcp_comm.header_buf[tcp_comm.received_size], recv_size);
		if ( ret < 0 ) 
		{
			DPN("recv(%d, tcp_header, %d) error %d", sock, recv_size, ret);
			return -1;
		}
		tcp_comm.received_size += ret;
	}

	if ( tcp_comm.received_size == TCP_HEADER_SIZE ) 
		return 1;

	return 0;
}

static void clear_tcp_header(void)
{
	memset(tcp_comm.header_buf, 0, sizeof(tcp_comm.header_buf));
	tcp_comm.received_size = 0;
}



int comm_tcp_client(int sock)
{
	static int prev_state = -1;
	int ret = 0;
	int state = getSn_SR(sock);

	switch(state)
	{
		case SOCK_ESTABLISHED:
			if ( prev_state != state )
				DPN("SOCK_ESTABLISHED");

			if ( wait_tcp_header(sock) ) 
			{
				printf("recv tcp header\r\n%s\r\n", tcp_comm.header_buf);

				ret = wiz_sendb(sock, tcp_comm.header_buf, TCP_HEADER_SIZE);
				printf("\r\nwiz_sendb, ret = %d\r\n", ret);
				clear_tcp_header();
			}
			break;

		case SOCK_CLOSE_WAIT:
			if ( prev_state != state )
				DPN("SOCK_CLOSE_WAIT");

    			ret = disconnect(sock);
			DPN("disconnect = %d", ret);
			break;

		case SOCK_CLOSED:
			if ( prev_state != state )
				DPN("SOCK_CLOSED");

			ret = close(sock);
			DPN("close = %d", ret);

			ret = socket(sock, Sn_MR_TCP, 8277, SOCK_IO_NONBLOCK);
			DPN("sock_tcp_client = %d", ret);
			break;

		case SOCK_INIT:
			if ( prev_state != state )
			{
				DPN("SOCK_INIT");
				DPN("ready to connect");
			}
			if ( comm.operation == OP_CONNECT_HOST ) 
			{
				ret = connect(sock, comm.host.ip, comm.host.port);
				DPN("connect host = %d", ret);
				comm.operation = OP_WAIT; 
			}
			break;

		default:
			break;
	}
	prev_state = state;
	return 0;
}

/********** end of file **********/
