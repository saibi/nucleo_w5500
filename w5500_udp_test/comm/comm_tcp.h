/*!
 * \file comm_tcp.h
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/30 13:58:52 (KST)
 *
*/
#ifndef _COMM_TCP_H_
#define _COMM_TCP_H_

#include <llist.h>

#define TCP_HEADER_SIZE 80

#define PREFIX_HEADER_ONLY "ewh "
#define PREFIX_HAVE_DATA "ewd "
#define PREFIX_LEN 4

enum tcp_packet_type
{
	TCP_PACKET_UNKNOWN = 0,
	TCP_PACKET_HEADER_ONLY,
	TCP_PACKET_HAVE_DATA,
};


int comm_tcp_client(int sock);

struct tcp_packet_rec 
{
	struct list_head list;

	int type;
	char header[TCP_HEADER_SIZE];
	int data_size;
	char data[ZERO_ARRAY_IDX];
};

#endif 
/********** end of file **********/
