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

#include <macro.h>
#include <helper.h>
#include <wiz_appl.h>
#include <tester.h>

#include <socket.h>

#include <stdlib.h>

struct _address_and_port {
	uint8_t addr[4];
	uint16_t port;
} host_info;

uint8_t recv_buf[MAX_WIZ_BUF];

static void setup_ip(wiz_NetInfo *netinfo)
{
	DPN("DHCP...");
	if ( get_DHCP_ip(netinfo->mac, netinfo) < 0 ) 
		DPN("ip error\n");
	else 
		pr_wiznetinfo(netinfo);

	wizchip_setnetinfo(netinfo);
}

#define EW_DGRAM_SIZE 64 

/// \return 1 tcp connection requested
static int handle_udp(int sock)
{
	uint8_t addr[4] = { 0, };
	uint16_t port = 0;
	int ret;

	ret = recvfrom(sock, recv_buf, sizeof(recv_buf) - 1, addr, &port);
	if ( ret > 0 ) 
	{
		printf("recv %d bytes from ", ret);
		prn_ip_port(addr, port);
		recv_buf[ret] = 0;
		printf("recv start ==========\r\n");
		printf("%s", recv_buf);
		printf("\r\nrecv end ==========\r\n");


		if ( strncmp((char*)recv_buf, "ew hello", 8) == 0 )
		{
			uint8_t dgram[EW_DGRAM_SIZE] = { 0, };

			DPN("ew hello received. reply ready");
			strncpy((char*)dgram, "ew ready w5500", sizeof(dgram) - 1);
			ret = sendto(sock, dgram, strlen((char*)dgram), addr, port);
			printf("sendto, ret = %d\r\n", ret);
		}
		else if ( strncmp((char*)recv_buf, "ew con ", 7 ) == 0 )
		{
			char * port_ptr = NULL;
			char * id_ptr = NULL;

			port_ptr = strtok((char*)&recv_buf[7], " ");
			if ( port_ptr )
				id_ptr = strtok(NULL, " ");
			
			if ( id_ptr )
			{
				DPN("ew con received. port [%s] , id [%s]", port_ptr, id_ptr);

				if ( strncmp(id_ptr, "w5500", 6) == 0 )
				{
					DPN("verify ok");
					memcpy(host_info.addr, addr, sizeof(addr));
					host_info.port = atoi(port_ptr);
					return 1;
				}
			}

		}
		else 
		{
			// echo
			DPN("echo udp datagram.");
			ret = sendto(sock, recv_buf, ret, addr, port);
			printf("sendto, ret = %d\r\n", ret);
		}

	}
	return 0;
}

static void handle_tcp_server(int sock)
{
	static int connected = 0;
	int ret = 0;

	switch(getSn_SR(sock))
	{
		case SOCK_ESTABLISHED:
			if ( !connected )
			{
				DPN("SOCK_ESTABLISHED");
				connected = 1;
			}

			ret = recv(sock, recv_buf, sizeof(recv_buf) - 1);
			if ( ret > 0 ) 
			{
				printf("recv %d bytes\r\n", ret);
				recv_buf[ret] = 0;
				printf("%s\r\n", recv_buf);

				ret = send(sock, recv_buf, ret);
				printf("sendto, ret = %d\r\n", ret);
			}

			break;

		case SOCK_CLOSE_WAIT:
			DPN("SOCK_CLOSE_WAIT");
			disconnect(sock);
			break;

		case SOCK_CLOSED:
			DPN("SOCK_CLOSED");
			close(sock);
			ret = socket(sock, Sn_MR_TCP, 8279, SOCK_IO_NONBLOCK);
			DPN("sock_tcp = %d", ret);
			break;

		case SOCK_INIT:
			DPN("SOCK_INIT");
			ret = listen(sock);
			DPN("listen tcp 8270 port = %d", ret);
			connected = 0;
			break;

		default:
			break;
	}
}

static void handle_tcp_client(int sock, int run_connect)
{
	static int prev_state = -1;
	int ret = 0;
	int state = getSn_SR(sock);

	switch(state)
	{
		case SOCK_ESTABLISHED:
			if ( prev_state != state )
				DPN("SOCK_ESTABLISHED");

			ret = recv(sock, recv_buf, sizeof(recv_buf) - 1);
			if ( ret > 0 ) 
			{
				printf("recv %d bytes\r\n", ret);
				recv_buf[ret] = 0;
				printf("%s\r\n", recv_buf);

				ret = send(sock, recv_buf, ret);
				printf("send, ret = %d\r\n", ret);
			}

			break;

		case SOCK_CLOSE_WAIT:
			if ( prev_state != state )
				DPN("SOCK_CLOSE_WAIT");

			disconnect(sock);
			break;

		case SOCK_CLOSED:
			if ( prev_state != state )
				DPN("SOCK_CLOSED");

			close(sock);
			ret = socket(sock, Sn_MR_TCP, 8277, SOCK_IO_NONBLOCK);
			DPN("sock_tcp_client = %d", ret);
			break;

		case SOCK_INIT:
			if ( prev_state != state )
			{
				DPN("SOCK_INIT");
				DPN("ready to connect");
			}
			if ( run_connect ) 
			{
				ret = connect(sock, host_info.addr, host_info.port);
				DPN("connect host = %d", ret);

				HAL_Delay(100);

				ret = recv(sock, recv_buf, sizeof(recv_buf) - 1);
				if ( ret > 0 ) 
				{
					printf("recv %d bytes\r\n", ret);
					recv_buf[ret] = 0;
					printf("%s\r\n", recv_buf);

					ret = send(sock, recv_buf, ret);
					printf("send, ret = %d\r\n", ret);
				}

			}
			break;

		default:
			break;
	}
	prev_state = state;
}

/// setup udp socket (2k buf)
/// \param port
/// \param block 0 - non blocking, 1 - blocking
/// \return socket number (0~7)
/// \return -1 socket unavailable
static int setup_udp_socket(int port, int block)
{
	int ret = 0;
	int sock = get_available_socket_no();
	if ( sock < 0 ) 
		return -1;

	ret = getSn_SR(sock);
	DPN("sock %d status = %d", sock, ret);

	ret = socket(sock, Sn_MR_UDP, port, block ? SOCK_IO_BLOCK : SOCK_IO_NONBLOCK);
	DPN("create udp socket, port %d, ret = %d", port, ret);
	ret = getSn_SR(sock);
	DPN("sock %d status = %d", sock, ret);

	return sock;
}

void event_loop(void)
{
	unsigned int prev_tick = 0;
	int sock_udp;
	int sock_tcp;
	int sock_tcp_client;
	int ret;

	wiz_NetInfo netinfo = {
		.mac = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0x44},
		.ip = {172, 10, 150, 131},
		.sn = {255, 255, 0, 0},
		.gw = {172, 10, 255, 254},
		.dns = {172, 16, 4, 3},
		.dhcp = NETINFO_STATIC,
	};

	malloc_test();

	setup_wizchip();
	setup_ip(&netinfo);


	sock_udp = setup_udp_socket(8279, 0);
	DPN("listen udp 8279 port");

	sock_tcp = 1;
	sock_tcp_client = 2;

	DPN("start main loop");
	while (1)
	{
		ret = handle_udp(sock_udp);
		handle_tcp_server(sock_tcp);
		handle_tcp_client(sock_tcp_client, ret);


		if ( run_per_x_seconds(&prev_tick, 10, CURRENT_TICK, TICKS_PER_SECOND) )
		{
			DPN("");
		}
	}
}
/********** end of file **********/
