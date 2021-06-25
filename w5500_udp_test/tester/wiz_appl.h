/*!
 * \file wiz_appl.h
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/18 10:23:18 (KST)
 *
*/
#ifndef _WIZ_APPL_H__
#define _WIZ_APPL_H__

#include <wizchip_conf.h>
#include <stdint.h>

#define MAX_WIZ_SOCKET 5 // use 5 sockets only

void wiz_init_chip(void);
int wiz_get_dhcp_ip(uint8_t mac[6], wiz_NetInfo *netinfo);
int wiz_get_available_socket_no(void);

int wiz_sendb(int sock, char *buf, int size);
int wiz_recvb(int sock, char *buf, int size);

void wiz_set_dhcp_ip(wiz_NetInfo *netinfo);

#endif 
/********** end of file **********/
