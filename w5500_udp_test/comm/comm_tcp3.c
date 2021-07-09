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

static unsigned short get_tcp_packet3_header_data_size(char *header)
{
	return convert_buf2short(&header[TCP_PACKET3_HEADER_IDX_DATASIZE0]);
}

static void tcp_packet3_header_to_field(char * header, struct tcp_packet3_rec * p)
{
	p->version = header[TCP_PACKET3_HEADER_IDX_VER];
	p->flag = header[TCP_PACKET3_HEADER_IDX_FLAG];
	p->type= header[TCP_PACKET3_HEADER_IDX_TYPE];
	p->data_size = get_tcp_packet3_header_data_size(header);
}

static void fill_tcp_packet3_data_field(struct tcp_packet3_rec *p)
{
	int offset = 0;

	p->checksum = 0;
	p->org_size = 0;

	if ( p->data_size == 0 )
		return;

	if ( p->flag & TCP_PACKET3_FLAG_BIT_CHECKSUM )
	{
		p->checksum = convert_buf2short(&p->data[offset]);
		offset += TCP_PACKET3_DATA_CHECKSUM_LEN;
	}

	if ( p->flag & TCP_PACKET3_FLAG_BIT_ENCRYPTION )
	{
		p->org_size = convert_buf2short(&p->data[offset]);
	}
	else 
	{
		p->org_size = p->data_size - offset;
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
	int offset = 0;

	printf("[%d|%d(0x%x)|%d|%d]", 
		p->version,
		p->flag, 
		p->flag,
		p->type,
		p->data_size
		);

	if ( p->flag & TCP_PACKET3_FLAG_BIT_CHECKSUM )
	{
		printf("[%d]", p->checksum);
		offset += TCP_PACKET3_DATA_CHECKSUM_LEN;
	}

	if ( p->flag & TCP_PACKET3_FLAG_BIT_ENCRYPTION )
	{
		printf("[%d]", p->org_size);
		offset += TCP_PACKET3_DATA_ORGSIZE_LEN;
	}

	printf("\r\n");

	if ( p->data_size > 0 && dump_contents_len > 0 )
	{
		int max = dump_contents_len;

		if ( max > p->data_size )
			max = p->data_size;

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
	}
	else 
	{
		DPN("insufficient memory");
		return NULL;
	}
	memcpy(p->header, header, TCP_PACKET3_HEADER_SIZE);
	tcp_packet3_header_to_field(header, p);

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
		fill_tcp_packet3_data_field(p);

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
	
	return (unsigned char)contents[0] + 0x0100; // test checksum
}

/// \return 1 checksum ok 
/// \return 0 checksum error
static int veryfy_tcp_packet3_checksum(struct tcp_packet3_rec *p)
{
	if ( p->data_size > 0 && p->flag & TCP_PACKET3_FLAG_BIT_CHECKSUM )
	{
		int calc = calc_checksum(&p->data[TCP_PACKET3_DATA_CHECKSUM_LEN], p->data_size - TCP_PACKET3_DATA_CHECKSUM_LEN);
		return (calc == p->checksum);
	}
	return 1;
}


static inline int is_packet_encrypted(struct tcp_packet3_rec * p)
{
	return (p->flag & TCP_PACKET3_FLAG_BIT_ENCRYPTION) ? 1 : 0;
}

static inline int get_packet_plain_contents_size(struct tcp_packet3_rec * p)
{
	return p->org_size;
}

static inline int get_packet_contents_size(struct tcp_packet3_rec * p)
{
	return (p->data_size - ((p->flag & TCP_PACKET3_FLAG_BIT_CHECKSUM) ? TCP_PACKET3_DATA_CHECKSUM_LEN : 0) - ((p->flag & TCP_PACKET3_FLAG_BIT_ENCRYPTION) ? TCP_PACKET3_DATA_ORGSIZE_LEN : 0 ));
}

static int encrypt_contents(char *in, int in_size, char *out)
{
	// TO-DO : encryption
	

	// test encription, append "encrypt "
	memcpy(out, "encrypt ", 8);
	memcpy(&out[8], in, in_size);

	return in_size + 8;
}

static int decrypt_contents(char *in, int in_size, char *out)
{
	// TO-DO : decryption
	

	// test decription, remove "encrypt "
	int out_size = in_size - 8;
	memcpy(out, &in[8], out_size); 
	return out_size;
}

static inline char * get_packet_contents_buf(struct tcp_packet3_rec * p)
{
	return &p->data[ ((p->flag & TCP_PACKET3_FLAG_BIT_CHECKSUM) ? TCP_PACKET3_DATA_CHECKSUM_LEN : 0 ) + ((p->flag & TCP_PACKET3_FLAG_BIT_ENCRYPTION) ? TCP_PACKET3_DATA_ORGSIZE_LEN : 0 ) ];
}

/// \param p packet pointer 
/// \return contents (dynamic allocated)
static char * extract_packet_contents(struct tcp_packet3_rec *p)
{
	int contents_size;
	char *buf;

	if ( p->data_size == 0 )
		return NULL;

	contents_size = get_packet_contents_size(p);

	if ( is_packet_encrypted(p) )
	{
		buf = (char*)malloc(get_packet_plain_contents_size(p) + 1);
		if ( buf == NULL )
		{
			DPN("insufficient memory.");
			return NULL;
		}
		decrypt_contents(get_packet_contents_buf(p), contents_size, buf);
	}
	else
	{
		buf = (char*)malloc(contents_size + 1);
		if ( buf == NULL )
		{
			DPN("insufficient memory.");
			return NULL;
		}
		memcpy(buf, get_packet_contents_buf(p), contents_size);
	}
	return buf;
}

#if 0
/// \param p packet pointer 
/// \return plain contents (dynamic allocated)
/// \return NULL plain packet
static char * extract_contents_from_encrypted_packet(struct tcp_packet3_rec *p)
{
	char * buf;

	if ( is_packet_encrypted(p) && get_packet_plain_contents_size(p) > 0 )
	{
		buf = (char*)malloc(get_packet_plain_contents_size(p) + 1);
		if ( buf == NULL )
		{
			DPN("insufficient memory.");
			return NULL;
		}

		decrypt_contents(get_packet_contents_buf(p), get_packet_contents_size(p), buf);
		return buf;
	}
	return NULL;
}
#endif 

static void fill_default_header(char *header)
{
	header[TCP_PACKET3_HEADER_IDX_FS] = RC_FS;
	header[TCP_PACKET3_HEADER_IDX_MAGIC] = RC_MAGIC;
	header[TCP_PACKET3_HEADER_IDX_VER] = TCP_PACKET3_VERSION;
	header[TCP_PACKET3_HEADER_IDX_FLAG] = TCP_PACKET3_FLAG_NONE;
	header[TCP_PACKET3_HEADER_IDX_TYPE] = TCP_PACKET3_TYPE_NONE;
	header[TCP_PACKET3_HEADER_IDX_DATASIZE0] = 0;
	header[TCP_PACKET3_HEADER_IDX_DATASIZE1] = 0;
	header[TCP_PACKET3_HEADER_IDX_GS] = RC_GS;
}

static int create_tcp_packet3_header(char * header, int flag, int type, int contents_size)
{
	int data_size = contents_size;

	fill_default_header(header);
	header[TCP_PACKET3_HEADER_IDX_FLAG] = flag & TCP_PACKET3_FIELD_MASK;
	header[TCP_PACKET3_HEADER_IDX_TYPE] = type & TCP_PACKET3_FIELD_MASK;

	if ( flag & TCP_PACKET3_FLAG_BIT_CHECKSUM ) 
		data_size += TCP_PACKET3_DATA_CHECKSUM_LEN;

	if ( flag & TCP_PACKET3_FLAG_BIT_ENCRYPTION ) 
		data_size += TCP_PACKET3_DATA_ORGSIZE_LEN;

	convert_short2buf(data_size, &header[TCP_PACKET3_HEADER_IDX_DATASIZE0]);

	return data_size;
}


static struct tcp_packet3_rec * create_tcp_packet3(int flag, int type, char *contents, int contents_size)
{
	char *buf = NULL;
	unsigned short buf_size = 0;
	unsigned short data_size = 0;
	struct tcp_packet3_rec * p;
	char header[TCP_PACKET3_HEADER_SIZE];
	int offset = 0;

	if ( contents_size > TCP_PACKET3_MAX_CONTENTS_SIZE ) 
	{
		DPN("too large contents");
		return NULL;
	}

	if ( contents != NULL && contents_size > 0 )
	{
		if ( flag & TCP_PACKET3_FLAG_BIT_ENCRYPTION ) 
		{

			// test encryption

			buf = (char *)malloc( contents_size + 8 );
			if ( buf == NULL )
			{
				DPN("insufficient memory");
				return NULL;
			}

			buf_size = encrypt_contents(contents, contents_size, buf);
			offset += TCP_PACKET3_DATA_ORGSIZE_LEN;
		}
		else 
		{
			buf = contents;
			buf_size = contents_size;
		}

		if ( flag & TCP_PACKET3_FLAG_BIT_CHECKSUM ) 
			offset += TCP_PACKET3_DATA_CHECKSUM_LEN;
	}

	data_size = create_tcp_packet3_header(header, flag, type, buf_size);
	p = alloc_tcp_packet3_mem(header, data_size);
	if ( p == NULL )
	{
		if ( flag & TCP_PACKET3_FLAG_BIT_ENCRYPTION ) 
			free(buf);

		return NULL;
	}

	if ( contents == NULL || contents_size <= 0 )
		return p;

	memcpy(&p->data[offset], buf, buf_size);
	p->org_size = contents_size;
	if ( flag & TCP_PACKET3_FLAG_BIT_ENCRYPTION ) 
	{
		free(buf);
		convert_short2buf(p->org_size, &p->data[offset - TCP_PACKET3_DATA_ORGSIZE_LEN]);
	}

	if ( flag & TCP_PACKET3_FLAG_BIT_CHECKSUM ) 
	{
		p->checksum = calc_checksum(&p->data[TCP_PACKET3_DATA_CHECKSUM_LEN], data_size - TCP_PACKET3_DATA_CHECKSUM_LEN );
		DPN("DBG checksum = %d", p->checksum);
		convert_short2buf(p->checksum, p->data);
	}

	return p;
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

	if ( packet->data_size > 0 )
	{
		ret = wiz_sendb(sock, packet->data, packet->data_size);
		if ( ret < 0 || ret != packet->data_size)
		{
			DPN("send data failed");
			return -2;
		}
	}
	return 0;
}

/// \return cmdline (dynamic allocated)
static char * get_cmdline_from_packet(struct tcp_packet3_rec *p)
{
	if ( p->type != TCP_PACKET3_TYPE_CMDLINE || p->data_size == 0 )
		return NULL;

	return extract_packet_contents(p);
}

/// \param pp_name (out) file name (dynamic allocated)
/// \param pp_file (out) file contents (dynamic allocated)
/// \return file size
static int get_file_from_packet(struct tcp_packet3_rec *p, char **pp_name, char **pp_file)
{
	char *buf;
	char *p_file;
	int size = 0;
	int info_size;
	int file_size;

	if ( p->type != TCP_PACKET3_TYPE_SMALLFILE || p->data_size == 0 )
		return 0;

	buf = extract_packet_contents(p);
	if ( buf == NULL )
		return 0;

	size = get_packet_contents_size(p);

	p_file = memchr(buf, '\0', size);
	if ( p_file == NULL )
	{
		free(buf);
		return 0;
	}
	p_file++;
	info_size = (p_file - buf);

	if ( pp_name )
	{
		*pp_name = (char*)malloc(info_size);
		if ( *pp_name == NULL )
		{
			DPN("insufficient memory");
			free(buf);
			return 0;
		}
		memcpy(*pp_name, buf, info_size);
	}

	file_size = size - info_size;

	if ( pp_file ) 
	{
		*pp_file = (char*)malloc(file_size);
		if ( *pp_file == NULL )
		{
			DPN("insufficient memory");
			free(buf);
			return 0;
		}
		memcpy(*pp_file, p_file, file_size);
	}
	free(buf);
	return file_size;
}

static void tcp_packet3_processor(int sock, struct list_head *phead)
{
	struct tcp_packet3_rec *p;
	char *buf;

	p = get_received_packet(phead);
	if ( p == NULL )
		return ;

	DPN("tcp_packet3 received. %d, %s", TCP_PACKET3_HEADER_SIZE + p->data_size, 
		veryfy_tcp_packet3_checksum(p) ? "ok" : "checksum err");

	print_tcp_packet3(p, 40);

	if ( is_packet_encrypted(p) )
	{
		struct tcp_packet3_rec * p_send;

		buf = extract_packet_contents(p);
		if ( buf ) 
		{
			DPN("decrypt data, org_size = %d", p->org_size);
			console_writeb(buf, 40);
			printf("\r\n");

			// packet send test

			// create plain packet
			p_send = create_tcp_packet3(0, 0, buf, p->org_size);
			if ( p_send )
			{
				DPN("echo plain packet");
				send_tcp_packet3(sock, p_send);

				free(p_send);
			}
			free(buf);
		}
	}

	switch (p->type)
	{
	case TCP_PACKET3_TYPE_CMDLINE:
		DPN("packet type = %d, echo packet", p->type);
		buf = get_cmdline_from_packet(p);
		if ( buf )
		{
			printf("%s\r\n",buf);
			free(buf);
		}
		
		send_tcp_packet3(sock, p);
		break;

	case TCP_PACKET3_TYPE_SMALLFILE:
	{
		char *filename = NULL;
		char *file = NULL;
		int file_size = 0;
		
		file_size = get_file_from_packet(p, &filename, &file); 
		if ( file_size > 0 ) 
		{
			DPN("small file, filesize = %d", file_size);
			DPN("%s", filename);
			DPN("%s", file);

			free(filename);
			free(file);
		}

	}
		break;

	default:
		break;
	}
	free(p);
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
			if ( ret == SOCKERR_SOCKINIT )
				comm.operation = OP_DHCP;

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
