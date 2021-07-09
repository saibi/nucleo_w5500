/*!
 * \file comm_udp.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/07/08 18:27:28 (KST)
 *
*/
#include "comm_udp.h"
#include "comm_base.h"
#include <macro.h>
#include <socket.h>
#include <stdlib.h>
#include <event_loop.h>
#include <wiz_appl.h>

int comm_udp_server(int sock)
{
	struct ip_port_rec sender;
	char recv_dgram[MAX_EW_DGRAM+1];
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

			ret = recvfrom(sock, (uint8_t*)recv_dgram, sizeof(recv_dgram) - 1, sender.ip, &sender.port);
			if ( ret > 0 ) 
			{
				printf("recv %d bytes from ", ret);
				prn_ip_port(&sender);
				recv_dgram[ret] = 0;
				console_writeb(recv_dgram, ret);
				printf("\r\n");

				if ( strncmp(recv_dgram, "ew hello", 8) == 0 )
				{
					char dgram[MAX_EW_DGRAM] = { 0, };

					DPN("ew hello received. reply ready");
					strncpy(dgram, "ew ready w5500", sizeof(dgram) - 1);
					ret = sendto(sock, (uint8_t *)dgram, strlen(dgram), sender.ip, sender.port);
					printf("sendto, ret = %d\r\n", ret);
				}
				else if ( strncmp(recv_dgram, "ew con ", 7 ) == 0 )
				{
					char * port_ptr = NULL;
					char * id_ptr = NULL;

					port_ptr = strtok(&recv_dgram[7], " ");
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
				else if ( strncmp(recv_dgram, "ew delay ", 9 ) == 0 )
				{
					set_event_loop_delay(atoi(&recv_dgram[9]));
				}
				else 
				{
					// echo
					DPN("echo udp datagram.");
					ret = sendto(sock, (uint8_t*)recv_dgram, ret, sender.ip, sender.port);
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

/********** end of file **********/
