/*!
 * \file comm_base.h
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/25 15:16:26 (KST)
 *
*/
#ifndef _COMM_BASE_H_
#define _COMM_BASE_H_

#include <global_def.h>
#include <helper.h>

enum operation_list {
	OP_WAIT, 
	OP_CONNECT_HOST,
};

struct comm_meta_rec {
	struct ip_port_rec host;
	char recv_buf[MAX_WIZ_BUF];

	int operation;
};

#define UDP_SERVER_PORT 8279
#define TCP_SERVER_PORT 8279

int comm_udp_server(int sock);
int comm_tcp_client(int sock);

#endif 
/********** end of file **********/
