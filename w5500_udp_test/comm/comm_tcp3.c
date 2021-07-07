/*!
 * \file comm_tcp3.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/07/02 13:51:54 (KST)
 *
*/
#include "comm_tcp3.h"
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

static unsigned short convert_buf2short(char *buf)
{
	union {
		char buf[2];
		unsigned short val;
	} conv;

	conv.buf[0] = buf[0];
	conv.buf[1] = buf[1];

	return conv.val;
}

#if 0
static void convert_short2buf(unsigned short val, char *buf)
{
	union {
		char buf[2];
		unsigned short val;
	} conv;

	conv.val = val;

	buf[0] = conv.buf[0];
	buf[1] = conv.buf[1];
}
#endif 

static int get_tcp_packet3_header_data_size(char *header)
{
	return convert_buf2short(&header[TCP_PACKET3_HEADER_IDX_DATASIZE0]);
}

static void tcp_packet3_header_to_field(char * header, struct tcp_packet3_field_rec * pfield)
{
	pfield->version = header[TCP_PACKET3_HEADER_IDX_VER];
	pfield->flag = header[TCP_PACKET3_HEADER_IDX_FLAG];
	pfield->type= header[TCP_PACKET3_HEADER_IDX_CMD];
	pfield->data_size = get_tcp_packet3_header_data_size(header);
}

static void tcp_packet3_to_field(struct tcp_packet3_rec *p, struct tcp_packet3_field_rec * pfield)
{
	int offset = 0;

	tcp_packet3_header_to_field(p->header, pfield);

	pfield->checksum = 0;
	if ( pfield->flag & TCP_PACKET3_FLAG_BIT_CHECKSUM )
	{
		pfield->checksum = convert_buf2short(&p->data[offset]);
		offset += TCP_PACKET3_DATA_CHECKSUM_LEN;
	}

	pfield->org_size = 0;
	if ( pfield->flag & TCP_PACKET3_FLAG_BIT_ENCRYPTION )
	{
		pfield->org_size = convert_buf2short(&p->data[offset]);
	}
}

#if 0
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
#endif 

static void print_tcp_packet3(struct tcp_packet3_rec *p, int dump_contents_len)
{
	struct tcp_packet3_field_rec field;
	int offset = 0;

	tcp_packet3_to_field(p, &field);

	printf("[%d|%d(0x%x)|%d|%d]", 
		field.version,
		field.flag, field.flag,
		field.type,
		field.data_size
		);

	if ( field.flag & TCP_PACKET3_FLAG_BIT_CHECKSUM )
	{
		printf("[%d]", field.checksum);
		offset += TCP_PACKET3_DATA_CHECKSUM_LEN;
	}

	if ( field.flag & TCP_PACKET3_FLAG_BIT_ENCRYPTION )
	{
		printf("[%d]", field.org_size);
		offset += TCP_PACKET3_DATA_ORGSIZE_LEN;
	}

	printf("\r\n");

	if ( field.data_size > 0 && dump_contents_len > 0 )
	{
		int max = dump_contents_len;

		if ( max > field.data_size )
			max = field.data_size;

		console_writeb(&p->data[offset], max);
		printf("\r\n");
	}
}

/// \return 1 yes
/// \return 0 no
static int is_tcp_packet3_header(char *header)
{
	if ( (header[TCP_PACKET3_HEADER_IDX_FS] == RC_FS) &&
		(header[TCP_PACKET3_HEADER_IDX_MAGIC] == RC_MAGIC) &&
		(header[TCP_PACKET3_HEADER_IDX_GS] == RC_GS ) 
		)
	{
		return 1;
	}
	return 0;
}

/// \return 1~7 tcp_pacet2 fragment index 
/// \return 0 not contains
static int contains_tcp_packet3_fragment(char *header)
{
	char * p;
	
	if ( ( p = memchr(&header[1], RC_FS, TCP_PACKET3_HEADER_SIZE-1) ) == NULL ) 
		return -1;

	if ( (int)(p - header) == (TCP_PACKET3_HEADER_SIZE - 1) ) 
		return TCP_PACKET3_HEADER_SIZE - 1;

	if ( p[1] == RC_MAGIC )
		return (int)(p - header);

	return -1;
}

static struct tcp_packet3_rec * alloc_tcp_packet3_mem(char *header, int data_size)
{
	struct tcp_packet3_rec * p;
	int mem_size;

	if ( data_size < 0 )
		data_size = 0;

	mem_size = sizeof(struct tcp_packet3_rec) + data_size + 4;

	p = (struct tcp_packet3_rec *) malloc( mem_size );
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
	memcpy(p->header, header, TCP_PACKET3_HEADER_SIZE);
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
			HAL_Delay(10); // need delay ???
		}
	} while ( received_size < size );
}

/// \return tcp_packet3 pointer
/// \return NULL
static struct tcp_packet3_rec * recv_tcp_packet3(int sock)
{
	enum recv_mode_idx
	{
		RECV_HEADER = 0,
		RECV_DATA,
	};

	static char header[TCP_PACKET3_HEADER_SIZE] = { 0, };
	static int received_size = 0;
	static int recv_mode = RECV_HEADER;
	static struct tcp_packet3_rec *packet;

	int ret;
	int recv_size;
	struct tcp_packet3_rec * p = NULL;
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
		
		p = packet;

		packet = NULL;
		clear_header = 1;
		recv_mode = RECV_HEADER;
		break;

	case RECV_HEADER:
	default:
		recv_size = TCP_PACKET3_HEADER_SIZE - received_size;
		ret = recv(sock, (uint8_t*)&header[received_size], recv_size);
		if ( ret < 0 ) 
		{
			DPN("recv(%d) header error %d", sock, ret);
			break;
		}

		received_size += ret;
		if ( received_size < TCP_PACKET3_HEADER_SIZE )
			break;

		if ( is_tcp_packet3_header(header) ) 
		{
			int data_size;

			clear_header = 1;

			data_size = get_tcp_packet3_header_data_size(header);
			p = alloc_tcp_packet3_mem(header, data_size);
			if ( p == NULL ) 
			{
				DPN("insufficient memory, ignore tcp data.");
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

			start_idx = contains_tcp_packet3_fragment(header);
			if ( start_idx == 0 ) 
			{
				clear_header = 1;
			}
			else 
			{
				memcpy(header, &header[start_idx], TCP_PACKET3_HEADER_SIZE - start_idx);
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

static unsigned short calc_checksum(char *contents, int contents_len)
{
	// TO-DO : calc checksum
	
	return contents[0] + 0x0100; // test checksum
}

/// \return 1 checksum ok 
/// \return 0 checksum error
static int veryfy_tcp_packet3_checksum(struct tcp_packet3_rec *packet)
{
	struct tcp_packet3_field_rec field;

	tcp_packet3_to_field(packet, &field);

	if ( field.flag & TCP_PACKET3_FLAG_BIT_CHECKSUM )
	{
		int calc = calc_checksum(&packet->data[TCP_PACKET3_DATA_CHECKSUM_LEN], field.data_size - TCP_PACKET3_DATA_CHECKSUM_LEN );

		return (calc == field.checksum);
	}

	return 1;
}

static void dump_tcp_packet3_list(struct list_head *phead)
{
	struct list_head * p;
	struct tcp_packet3_rec *entry = NULL;
	int cnt = 0;

	if ( list_count(phead) > 0 )
	{
		DPN("tcp_packet3_list ==========");
		list_for_each(p, phead)
		{
			entry = list_entry(p, struct tcp_packet3_rec, list);
			printf("#%d ", cnt++);
			print_tcp_packet3(entry, 8);
		}
	}
}

static void free_tcp_packet3_list(struct list_head *phead)
{
	struct list_head * p;
	struct tcp_packet3_rec *entry = NULL;

	list_for_each(p, phead)
	{
		if ( entry )
			free(entry);

		entry = list_entry(p, struct tcp_packet3_rec, list);
	}

	if ( entry )
		free(entry);

	INIT_LIST_HEAD(phead);
}

static struct tcp_packet3_rec * get_received_packet(struct list_head * phead)
{
	struct list_head * p;
	struct tcp_packet3_rec *entry = NULL;

	list_for_each(p, phead)
	{
		entry = list_entry(p, struct tcp_packet3_rec, list);
		break;
	}

	if ( entry )
		list_del(&entry->list);

	return entry;
}

/// \return 0 success
/// \return -1 header error
/// \return -2 data error
static int send_tcp_packet3(int sock, struct tcp_packet3_rec *packet)
{
	int ret;

	ret = wiz_sendb(sock, packet->header, TCP_PACKET3_HEADER_SIZE);
	if ( ret < 0 || ret != TCP_PACKET3_HEADER_SIZE )
	{
		DPN("send header failed");
		return -1;
	}

	ret = wiz_sendb(sock, packet->data, packet->data_size);
	if ( ret < 0 || ret != packet->data_size)
	{
		DPN("send data failed");
		return -2;
	}
	return 0;
}

static void tcp_packet3_processor(int sock, struct list_head *phead)
{
	struct tcp_packet3_rec *packet;

	packet = get_received_packet(phead);

	if ( packet == NULL )
		return ;

	DPN("tcp_packet3 received. %d, %s", TCP_PACKET3_HEADER_SIZE + packet->data_size, 
		veryfy_tcp_packet3_checksum(packet) ? "ok" : "checksum err");
	print_tcp_packet3(packet, 40);

	if ( packet->data_size > 0 )
	{
		DPN("echo packet");
		send_tcp_packet3(sock, packet);
	} 
	free(packet);
}

void comm_tcp_packet_handler(int sock)
{
	tcp_packet3_processor(sock, &packet_list);
}

/// \return 1 packet received
/// \return 0 
int comm_tcp_client(int sock)
{
	static int prev_state = -1;
	int state = getSn_SR(sock);
	int ret = 0;
	struct tcp_packet3_rec *p;
	int is_packet_received = 0;

	switch(state)
	{
		case SOCK_ESTABLISHED:
			if ( prev_state != state )
				DPN("SOCK_ESTABLISHED");

			p = recv_tcp_packet3(sock);
			if ( p )
			{
				list_add_tail(&p->list, &packet_list);
				is_packet_received = 1;
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

			dump_tcp_packet3_list(&packet_list);
			free_tcp_packet3_list(&packet_list);

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
	return is_packet_received;
}

/********** end of file **********/
