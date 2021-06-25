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
#include <macro.h>
#include <socket.h>
#include <stdlib.h>
#include <event_loop.h>
#include <wiz_appl.h>

#define MAX_EW_DGRAM 64 

struct comm_meta_rec comm;

int comm_udp_server(int sock)
{
	struct ip_port_rec sender;
	int ret;
	static int prev_state = -1;
	int state = getSn_SR(sock);

	switch(state)
	{
		case SOCK_CLOSED:
			if ( prev_state != state )
				DPN("SOCK_CLOSED");

			ret = socket(sock, Sn_MR_UDP, UDP_SERVER_PORT, SOCK_IO_NONBLOCK);
			DPN("socket = %d", ret);
			DPN("listen udp %d port", UDP_SERVER_PORT);
			break;

		case SOCK_UDP:
			if ( prev_state != state )
				DPN("SOCK_UDP");

			ret = recvfrom(sock, (uint8_t*)comm.recv_buf, sizeof(comm.recv_buf) - 1, sender.ip, &sender.port);
			if ( ret > 0 ) 
			{
				printf("recv %d bytes from ", ret);
				prn_ip_port(&sender);
				comm.recv_buf[ret] = 0;
				console_writeb(comm.recv_buf, ret);
				printf("\r\n");

				if ( strncmp((char*)comm.recv_buf, "ew hello", 8) == 0 )
				{
					uint8_t dgram[MAX_EW_DGRAM] = { 0, };

					DPN("ew hello received. reply ready");
					strncpy((char*)dgram, "ew ready w5500", sizeof(dgram) - 1);
					ret = sendto(sock, dgram, strlen((char*)dgram), sender.ip, sender.port);
					printf("sendto, ret = %d\r\n", ret);
				}
				else if ( strncmp(comm.recv_buf, "ew con ", 7 ) == 0 )
				{
					char * port_ptr = NULL;
					char * id_ptr = NULL;

					port_ptr = strtok(&comm.recv_buf[7], " ");
					if ( port_ptr )
						id_ptr = strtok(NULL, " ");
					
					if ( id_ptr )
					{
						DPN("ew con received. port [%s] , id [%s]", port_ptr, id_ptr);

						if ( strncmp(id_ptr, "w5500", 6) == 0 )
						{
							DPN("verify ok");
							COPY_IP_ADDR(comm.host.ip, sender.ip);
							comm.host.port = atoi(port_ptr);
							comm.operation = OP_CONNECT_HOST;
						}
					}

				}
				else if ( strncmp(comm.recv_buf, "ew delay ", 9 ) == 0 )
				{
					set_event_loop_delay(atoi(&comm.recv_buf[9]));
				}
				else 
				{
					// echo
					DPN("echo udp datagram.");
					ret = sendto(sock, (uint8_t*)comm.recv_buf, ret, sender.ip, sender.port);
					printf("sendto, ret = %d\r\n", ret);
				}
			}
			break;

		default:
			break;
	}
	prev_state = state;
	return 0;
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

			ret = recv(sock, (uint8_t*)comm.recv_buf, sizeof(comm.recv_buf) - 1);
			if ( ret > 0 ) 
			{
				printf("recv %d bytes\r\n", ret);
				comm.recv_buf[ret] = 0;
				
				console_writeb(comm.recv_buf, ret > 40 ? 40 : ret);

				ret = wiz_sendb(sock, comm.recv_buf, ret);
				printf("\r\nwiz_sendb, ret = %d\r\n", ret);
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
