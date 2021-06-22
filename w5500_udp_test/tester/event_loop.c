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

#include <socket.h>

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

#define EW_DGRAM_SIZE 8

static void handle_udp(int sock)
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

			memcpy(dgram, "ew ready", EW_DGRAM_SIZE);

			ret = sendto(sock, dgram, EW_DGRAM_SIZE, addr, port);
			printf("sendto, ret = %d\r\n", ret);
		}
		else 
		{
			// echo
			DPN("echo udp datagram.");
			ret = sendto(sock, recv_buf, ret, addr, port);
			printf("sendto, ret = %d\r\n", ret);
		}


	}
}

static void handle_tcp(int sock)
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


void event_loop(void)
{
	unsigned int prev_tick = 0;
	int sock_udp;
	int sock_tcp;

	wiz_NetInfo netinfo = {
		.mac = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0x44},
		.ip = {172, 10, 150, 131},
		.sn = {255, 255, 0, 0},
		.gw = {172, 10, 255, 254},
		.dns = {172, 16, 4, 3},
		.dhcp = NETINFO_STATIC,
	};


	setup_wizchip();
	setup_ip(&netinfo);


	sock_udp = setup_udp_socket(8279, 0);
	DPN("listen udp 8279 port");

	sock_tcp = 1;

	DPN("start main loop");
	while (1)
	{
		handle_udp(sock_udp);
		handle_tcp(sock_tcp);

		if ( run_per_x_seconds(&prev_tick, 5, CURRENT_TICK, TICKS_PER_SECOND) )
		{

			DPN("");
		}
	}
}
/********** end of file **********/
