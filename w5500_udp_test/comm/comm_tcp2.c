/*!
 * \file comm_tcp2.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/07/02 13:51:54 (KST)
 *
*/
#include "comm_tcp2.h"
#include "comm_base.h"
#include <macro.h>
#include <str_helper.h>
#include <wiz_appl.h>
#include <helper.h>
#include <socket.h>
#include <stdlib.h>
#include <malloc.h>

extern struct comm_meta_rec comm;

LIST_HEAD(packet_list);

static int get_tcp_packet2_header_data_size(char *header)
{
	union {
		unsigned char buf[2];
		unsigned short val;
	} data_size;

	data_size.buf[0] = header[TCP_PACKET2_HEADER_IDX_DATASIZE0];
	data_size.buf[1] = header[TCP_PACKET2_HEADER_IDX_DATASIZE1];

	return data_size.val;
}

static void tcp_packet2_header_to_field(char * header, struct tcp_packet2_field_rec * pfield)
{
	pfield->version = header[TCP_PACKET2_HEADER_IDX_VER];
	pfield->flag = header[TCP_PACKET2_HEADER_IDX_FLAG];
	pfield->cmd = header[TCP_PACKET2_HEADER_IDX_CMD];
	pfield->data_size = get_tcp_packet2_header_data_size(header);
}


static void tcp_packet2_data_to_field(char *data, struct tcp_packet2_field_rec * pfield)
{
	pfield->checksum_type = data[TCP_PACKET2_DATA_IDX_CHECKSUM_TYPE];
	pfield->checksum_size = data[TCP_PACKET2_DATA_IDX_CHECKSUM_SIZE];
}

static void tcp_packet2_to_field(struct tcp_packet2_rec *p, struct tcp_packet2_field_rec * pfield)
{
	tcp_packet2_header_to_field(p->header, pfield);
	tcp_packet2_data_to_field(p->data, pfield);
	pfield->contents_size = (p->data_size > 0) ? p->data_size - TCP_PACKET2_DATA_IDX_CHECKSUM_START - pfield->checksum_size : 0;
}


#if 0
static void set_tcp_packet2_header_data_size(char *header, int value)
{
	union {
		unsigned char buf[2];
		unsigned short val;
	} data_size;

	data_size.val = value;

	header[TCP_PACKET2_HEADER_IDX_DATASIZE0] = data_size.buf[0]; 
	header[TCP_PACKET2_HEADER_IDX_DATASIZE1] = data_size.buf[1];
}

static void tcp_packet2_field_to_header(struct tcp_packet2_field_rec * pfield, char *header)
{
	header[TCP_PACKET2_HEADER_IDX_FS] = RC_FS;
	header[TCP_PACKET2_HEADER_IDX_MAGIC] = RC_MAGIC;
	header[TCP_PACKET2_HEADER_IDX_VER] = pfield->version;
	header[TCP_PACKET2_HEADER_IDX_FLAG] = pfield->flag;
	header[TCP_PACKET2_HEADER_IDX_CMD] = pfield->cmd;
	header[TCP_PACKET2_HEADER_IDX_GS] = RC_GS;
	set_tcp_packet2_header_data_size(header, pfield->data_size);
}
#endif 

static void char2hexstr(char val, char hexstr[2])
{
	int hex;
	int i;

	for ( i = 0 ; i < 2 ; ++i )
	{
		hex = (val >> (4 - 4 * i)) & 0x0f;

		if ( hex < 10 )
			hex += '0';
		else 
			hex = (hex - 10) + 'a';

		hexstr[i] = hex;
	}
}

static void dump_hex_val(char *buf, int size)
{
	char dump[2];
	int i;

	for ( i = 0 ; i < size; ++i )
	{
		char2hexstr(buf[i], dump);
		console_writeb(dump, sizeof(dump));
	}
}

static void print_tcp_packet2(struct tcp_packet2_rec *p, int dump_contents_len)
{
	struct tcp_packet2_field_rec field;

	tcp_packet2_to_field(p, &field);

	printf("[%d|%d(0x%x)|%d|%d][%d|%d|(%d)]", 
		field.version,
		field.flag, field.flag,
		field.cmd,
		field.data_size,
		field.checksum_type,
		field.checksum_size,
		field.contents_size 
		);
	
	printf("\r\n");
	if ( field.checksum_type != CHECKSUM_NONE && field.checksum_size > 0) 
	{
		printf("<");
		dump_hex_val(&p->data[TCP_PACKET2_DATA_IDX_CHECKSUM_START], field.checksum_size);
		printf(">\r\n");
	}

	if ( field.data_size > 0 && dump_contents_len > 0 )
	{
		int contents_idx = TCP_PACKET2_DATA_IDX_CHECKSUM_START + field.checksum_size;
		console_writeb(&p->data[contents_idx], dump_contents_len);
		printf("\r\n");
	}
}

/// \return 1 yes
static int is_tcp_packet2_header(char *header)
{
	if ( (header[TCP_PACKET2_HEADER_IDX_FS] == RC_FS) &&
		(header[TCP_PACKET2_HEADER_IDX_MAGIC] == RC_MAGIC) &&
		(header[TCP_PACKET2_HEADER_IDX_GS] == RC_GS ) 
		)
	{
		return 1;
	}

	return 0;
}

/// \return 1~7 tcp_pacet2 fragment index 
/// \return 0 not contains
static int contains_tcp_packet2_fragment(char *header)
{
	char * p;
	
	if ( ( p = memchr(&header[1], RC_FS, TCP_PACKET2_HEADER_SIZE-1) ) == NULL ) 
		return -1;

	if ( (int)(p - header) == (TCP_PACKET2_HEADER_SIZE - 1) ) 
		return TCP_PACKET2_HEADER_SIZE - 1;

	if ( p[1] == RC_MAGIC )
		return (int)(p - header);

	return -1;
}

static struct tcp_packet2_rec * alloc_tcp_packet2_mem(char *header, int data_size)
{
	struct tcp_packet2_rec * p;
	int mem_size;

	if ( data_size < 0 )
		data_size = 0;

	mem_size = sizeof(struct tcp_packet2_rec) + data_size + 4;

	p = (struct tcp_packet2_rec *) malloc( mem_size );
	if ( p )
	{
		memset(p, 0, mem_size); 
		p->data_size = data_size;
	}
	else 
	{
		DPN("insufficient memory");
		return NULL;
	}
	memcpy(p->header, header, TCP_PACKET2_HEADER_SIZE);

	return p;
}

static void eat_tcp_data(int sock, int size)
{
	char buf[128];
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
		else if ( ret < 0 )
		{
			DPN("recv ret = %d", ret);
			HAL_Delay(10);
		}
	} while ( received_size < size );
}

/// \return tcp packet2 pointer
/// \return NULL
static struct tcp_packet2_rec * recv_tcp_packet2(int sock)
{
	enum recv_mode_idx
	{
		RECV_HEADER = 0,
		RECV_DATA,
	};

	static char header[TCP_PACKET2_HEADER_SIZE] = { 0, };
	static int received_size = 0;
	static int recv_mode = RECV_HEADER;
	static struct tcp_packet2_rec *packet;

	int ret;
	int recv_size;
	struct tcp_packet2_rec * p = NULL;
	int clear_header = 0;

	switch(recv_mode) 
	{
	case RECV_DATA:
		recv_size = packet->data_size - received_size;

		ret = recv(sock, (uint8_t*)&packet->data[received_size], recv_size);
		if ( ret < 0 )
		{
			DPN("recv(%d) data error %d", sock, ret);
			break;
		}

		received_size += ret;
		if ( received_size < packet->data_size )
			break;

		// TO-DO : handle checksum
		
		p = packet;

		packet = NULL;
		clear_header = 1;
		recv_mode = RECV_HEADER;
		break;

	case RECV_HEADER:
	default:

		recv_size = TCP_PACKET2_HEADER_SIZE - received_size;

		ret = recv(sock, (uint8_t*)&header[received_size], recv_size);
		if ( ret < 0 ) 
		{
			DPN("recv(%d) header error %d", sock, ret);
			break;
		}

		received_size += ret;
		if ( received_size < TCP_PACKET2_HEADER_SIZE )
			break;

		if ( is_tcp_packet2_header(header) ) 
		{
			int data_size;

			clear_header = 1;

			data_size = get_tcp_packet2_header_data_size(header);
			p = alloc_tcp_packet2_mem(header, data_size);
			if ( p == NULL ) 
			{
				eat_tcp_data(sock, data_size);
				break;
			}

			if ( data_size > 0 )
			{
				packet = p;
				p = NULL;
				recv_mode = RECV_DATA;
			}
		}
		else
		{
			int start_idx = 0;

			start_idx = contains_tcp_packet2_fragment(header);
			if ( start_idx == 0 ) 
			{
				clear_header = 1;
			}
			else 
			{
				memcpy(header, &header[start_idx], TCP_PACKET2_HEADER_SIZE - start_idx);
				received_size -= start_idx;
			}
		}

		break;
	}

	if ( clear_header )
	{
		memset(header, 0, sizeof(header));
		received_size = 0;
	}
	return p;
}

static void dump_tcp_packet2_list(struct list_head *phead)
{
	struct list_head * p;
	struct tcp_packet2_rec *entry = NULL;
	int cnt = 0;

	if ( list_count(phead) > 0 )
	{
		DPN("tcp_packet2_list ==========");
		list_for_each(p, phead)
		{
			entry = list_entry(p, struct tcp_packet2_rec, list);
			printf("#%d ", cnt++);
			print_tcp_packet2(entry, 8);
		}
	}
}

static void free_tcp_packet2_list(struct list_head *phead)
{
	struct list_head * p;
	struct tcp_packet2_rec *entry = NULL;

	list_for_each(p, phead)
	{
		if ( entry )
			free(entry);

		entry = list_entry(p, struct tcp_packet2_rec, list);
	}

	if ( entry )
		free(entry);

	INIT_LIST_HEAD(phead);
}

int comm_tcp_client(int sock)
{
	static int prev_state = -1;
	int state = getSn_SR(sock);
	int ret = 0;
	struct tcp_packet2_rec *p;

	switch(state)
	{
		case SOCK_ESTABLISHED:
			if ( prev_state != state )
				DPN("SOCK_ESTABLISHED");

			p = recv_tcp_packet2(sock);
			if ( p )
			{
				DPN("tcp_packet2 received. %d", TCP_PACKET2_HEADER_SIZE + p->data_size);
				print_tcp_packet2(p, 40);

				list_add_tail(&p->list, &packet_list);
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

			dump_tcp_packet2_list(&packet_list);
			free_tcp_packet2_list(&packet_list);

			int port = gen_random_port();

			ret = socket(sock, Sn_MR_TCP, port, SOCK_IO_NONBLOCK);
			DPN("sock_tcp_client = %d, port = %d", ret, port);
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
