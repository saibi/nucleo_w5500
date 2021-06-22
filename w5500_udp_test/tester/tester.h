/*!
 * \file tester.h
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/05/17 10:33:15 (KST)
 *
*/
#ifndef _TESTER_H_
#define _TESTER_H_

#include <stdint.h>

// tcp_echo_client.c
int tcp_echo_test(void);

// udp_echo_client.c
int udp_echo_test(uint8_t dest_addr[4], int dest_port, char * send_str);

// udp_echo_server.c
int udp_echo_server_loop(int loop_seconds);

#endif 

/********** end of file **********/
