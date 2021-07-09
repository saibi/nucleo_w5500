/*!
 * \file comm_tcp3.h
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/07/02 14:01:42 (KST)
 *
*/

#ifndef _COMM_TCP3_H_
#define _COMM_TCP3_H_

#include <llist.h>

enum tcp_packet3_constants {
	TCP_PACKET3_HEADER_IDX_FS = 0,
	TCP_PACKET3_HEADER_IDX_MAGIC,
	TCP_PACKET3_HEADER_IDX_VER,
	TCP_PACKET3_HEADER_IDX_FLAG,
	TCP_PACKET3_HEADER_IDX_TYPE,
	TCP_PACKET3_HEADER_IDX_DATASIZE0,
	TCP_PACKET3_HEADER_IDX_DATASIZE1,
	TCP_PACKET3_HEADER_IDX_GS,
	TCP_PACKET3_HEADER_SIZE, // 8 

	TCP_PACKET3_DATA_CHECKSUM_LEN = 2,
	TCP_PACKET3_DATA_SIZE_LEN = 2,

	TCP_PACKET3_MAX_CONTENTS_SIZE = 50000,

	// version
	TCP_PACKET3_VERSION = 1,

	// flag bit
	TCP_PACKET3_FLAG_NONE = 0,
	TCP_PACKET3_FLAG_BIT_CHECKSUM = 0x80,
	TCP_PACKET3_FLAG_BIT_ENCRYPTION = 0x01,

	// type 
	TCP_PACKET3_TYPE_NONE = 0,
	TCP_PACKET3_TYPE_CMDLINE, 
	TCP_PACKET3_TYPE_SMALLFILE ,


	// reserved characters
	RC_MAGIC = 0x42,
	RC_FS = 0x1c,
	RC_GS = 0x1d,
	
	TCP_PACKET3_FIELD_MASK = 0xff,
};


struct tcp_packet3_rec {

	struct list_head list;

	// from header
	unsigned char version;
	unsigned char flag;
	unsigned char type;
	unsigned short data_size;

	// from data (data_size > 0)
	unsigned short checksum;
	unsigned short org_size;

	// raw buffer
	char header[TCP_PACKET3_HEADER_SIZE];
	char data[ZERO_ARRAY_IDX]; // dynamic allocated
};


int comm_tcp_client(int sock);
void comm_tcp_packet_handler(int sock);

#endif 
/********** end of file **********/

