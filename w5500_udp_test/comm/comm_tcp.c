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
#include <str_helper.h>
#include <wiz_appl.h>
#include <socket.h>
#include <stdlib.h>
#include <malloc.h>

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

/// \param header header buffer
/// \param data_size (out) return data_size if packet is TCP_PACKET_HAVE_DATA
/// \return packet_type 
static int check_tcp_header(char *header, int *data_size)
{
	char size_val[TCP_HEADER_SIZE] = { 0, };

	if ( strncmp(PREFIX_HAVE_DATA, header, PREFIX_LEN) == 0 ) 
	{
		if ( data_size )
		{
			if ( extract_token(header, " ", 2, size_val, sizeof(size_val)) )
			{
				*data_size = atoi(size_val);
			}
		}
		return TCP_PACKET_HAVE_DATA;
	}
	else if ( strncmp(PREFIX_HEADER_ONLY, header, PREFIX_LEN) == 0 ) 
	{
		return TCP_PACKET_HEADER_ONLY;
	}

	return TCP_PACKET_UNKNOWN;
}


static struct tcp_packet_rec * alloc_tcp_packet(int type, int data_size)
{
	struct tcp_packet_rec *p;
	int mem_size = sizeof(struct tcp_packet_rec) + data_size + 4;

	p = (struct tcp_packet_rec * )malloc(mem_size);
	if ( !p ) 
	{
		DPN("insufficient memory");
	}
	else
	{
		memset(p, 0, mem_size);

		p->type = type;
		p->data_size = data_size;
	}
	return p;
}

LIST_HEAD(tcp_packet_list);

static void free_tcp_packet_list(void)
{
	struct list_head * p;
	struct tcp_packet_rec *entry = NULL;

	list_for_each(p, &tcp_packet_list) 
	{
		if ( entry )
			free(entry);

		entry = list_entry(p, struct tcp_packet_rec, list);
	}

	if ( entry )
		free(entry);

	INIT_LIST_HEAD(&tcp_packet_list);
}

static void dump_tcp_packet_list(void)
{
	struct list_head * p;
	struct tcp_packet_rec *entry = NULL;

	DPN("tpc_packet_list ==========");
	list_for_each(p, &tcp_packet_list) 
	{
		entry = list_entry(p, struct tcp_packet_rec, list);
		DPN("%s", entry->header);
	}
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
				int type;
				int data_size = 0;
				
				type = check_tcp_header(tcp_comm.header_buf, &data_size );

				printf("recv tcp header\r\n%s\r\n", tcp_comm.header_buf);

				if ( type != TCP_PACKET_UNKNOWN )
				{
					struct tcp_packet_rec *p;

					p = alloc_tcp_packet(type, data_size);
					if ( p ) 
					{
						memcpy(p->header, tcp_comm.header_buf, TCP_HEADER_SIZE);

						list_add(&p->list, &tcp_packet_list);
					}
					

				}


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

			dump_tcp_packet_list();
			free_tcp_packet_list();
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
