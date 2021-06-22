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
#include <string.h>

#define MAX_WIZ_SOCKET 3 // use 3 sockets only
#define MAX_WIZ_BUF (8*1024)


void setup_wizchip(void);
int get_DHCP_ip(uint8_t mac[6], wiz_NetInfo *netinfo);
int get_socket_buf_size(int sock);
int get_available_socket_no(void);

#define COPY_MAC_ADDR(dest, src) memcpy(dest, src, sizeof(uint8_t)*6)

#endif 
/********** end of file **********/
