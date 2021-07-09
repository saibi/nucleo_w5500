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
	OP_DHCP,
	OP_WAIT, 
	OP_CONNECT_HOST,
};

struct comm_meta_rec {
	struct ip_port_rec host;

	int operation;
};

void comm_handler(void);

extern struct comm_meta_rec comm;

#endif 
/********** end of file **********/
