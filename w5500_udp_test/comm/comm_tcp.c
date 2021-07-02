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

static int recv_remain_tcp_header(int sock)
{
	char *p; 

	p = strstr(tcp_comm.header_buf, PREFIX_HEADER_ONLY);
	if ( !p ) 
		p = strstr(tcp_comm.header_buf, PREFIX_HAVE_DATA);

	if ( !p )
	{
		DPN("invalid header");
		console_writeb(tcp_comm.header_buf, TCP_HEADER_SIZE);
		printf("\r\n");
		clear_tcp_header();
		return 0;
	} 
	else 
	{
		int offset = (p - tcp_comm.header_buf);
		int ret;

		memcpy(tcp_comm.header_buf, p, TCP_HEADER_SIZE - offset);
		ret = wiz_recvb(sock, &tcp_comm.header_buf[offset], offset);
		DPN("recv remain header %d bytes", ret);
	}

	return 1;
}

static void eat_tcp_data(int sock, int size)
{
	char buf[10];
	int read_size = sizeof(buf);
	int remain = size;
	int received_size = 0;
	int ret;

	do 
	{
		if ( remain < read_size )
			read_size = remain;

		ret = recv(sock, (uint8_t *)buf, read_size);
		if ( ret > 0 )
		{
			received_size += ret;
			remain -= ret;
		}
		else 
		{
			DPN("recv ret = %d", ret);
			HAL_Delay(10);
		}
	} while ( received_size < size );
}


#define WAIT 0
#define RECV_REMAIN 1

int comm_tcp_client(int sock)
{
	static int prev_state = -1;
	int ret = 0;
	int state = getSn_SR(sock);
	static int recv_mode = WAIT;
	int is_received = 0;

	switch(state)
	{
		case SOCK_ESTABLISHED:
			if ( prev_state != state )
				DPN("SOCK_ESTABLISHED");

			if ( recv_mode == WAIT)
			{
				is_received = wait_tcp_header(sock);
			}
			else if ( recv_mode == RECV_REMAIN ) 
			{
				is_received = recv_remain_tcp_header(sock);
				recv_mode = WAIT;
			}

			if ( is_received == 1 ) 
			{
				int type;
				int data_size = 0;
				
				type = check_tcp_header(tcp_comm.header_buf, &data_size );

				if ( type != TCP_PACKET_UNKNOWN )
				{
					struct tcp_packet_rec *p;

					printf("recv tcp header\r\n%s\r\ntype=%d\r\n", tcp_comm.header_buf, type);

					p = alloc_tcp_packet(type, data_size);
					if ( !p ) 
					{
						if ( data_size > 0 ) 
						{
							DPN("eat data");
							eat_tcp_data(sock, data_size);
						}
						break;
					}


					memcpy(p->header, tcp_comm.header_buf, TCP_HEADER_SIZE);
					if ( type == TCP_PACKET_HAVE_DATA ) 
					{
						ret = wiz_recvb(sock, p->data, data_size);
						DPN("recv tcp data %d bytes", ret);
						if ( ret != data_size ) 
						{
							// rebuild header 
							DPN("data_size = %d, only %d bytes received", data_size, ret);
							memset(p->header, 0, TCP_HEADER_SIZE);
							sprintf(p->header, PREFIX_HAVE_DATA "%d d org data_size %d, only %d bytes received.", ret, data_size, ret);
							p->data_size = ret;
						}
					}
					list_add_tail(&p->list, &tcp_packet_list);


					// echo packet

					ret = wiz_sendb(sock, p->header, TCP_HEADER_SIZE);
					printf("\r\nwiz_sendb header, ret = %d\r\n", ret);

					if ( type == TCP_PACKET_HAVE_DATA ) 
					{
						ret = wiz_sendb(sock, p->data, p->data_size);
						printf("\r\nwiz_sendb data, ret = %d\r\n", ret);
					}

					clear_tcp_header();
				} 
				else
				{
					DPN("unknown packet. mode = %d", recv_mode);
					recv_mode = RECV_REMAIN;
				}
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

			dump_tcp_packet_list();
			free_tcp_packet_list();

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
