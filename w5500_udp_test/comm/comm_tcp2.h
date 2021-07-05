/*!
 * \file comm_tcp2.h
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/07/02 14:01:42 (KST)
 *
*/

#ifndef _COMM_TCP2_H_
#define _COMM_TCP2_H_

#include <llist.h>

int comm_tcp_client(int sock);


enum tcp_packet2_constants {
	TCP_PACKET2_HEADER_IDX_FS = 0,
	TCP_PACKET2_HEADER_IDX_MAGIC,
	TCP_PACKET2_HEADER_IDX_VER,
	TCP_PACKET2_HEADER_IDX_FLAG,
	TCP_PACKET2_HEADER_IDX_CMD,
	TCP_PACKET2_HEADER_IDX_DATASIZE0,
	TCP_PACKET2_HEADER_IDX_DATASIZE1,
	TCP_PACKET2_HEADER_IDX_GS,
	TCP_PACKET2_HEADER_SIZE, // 8 

	TCP_PACKET2_DATA_IDX_CHECKSUM_TYPE = 0,
	TCP_PACKET2_DATA_IDX_CHECKSUM_SIZE,
	TCP_PACKET2_DATA_IDX_CHECKSUM_START,

	TYPE_NONE = 0,
	CMD_NONE = 0,
	CHECKSUM_NONE = 0,

	// reserved characters
	RC_MAGIC = 0x42,
	RC_FS = 0x1c,
	RC_GS = 0x1d,
};


struct tcp_packet2_rec {

	struct list_head list;

	int data_size;
	char header[TCP_PACKET2_HEADER_SIZE];
	char data[ZERO_ARRAY_IDX];
};

struct tcp_packet2_field_rec {
	int version;
	int flag;
	int cmd;
	int data_size;
	int checksum_type;
	int checksum_size;
	int contents_size;
};

#endif 
/********** end of file **********/

