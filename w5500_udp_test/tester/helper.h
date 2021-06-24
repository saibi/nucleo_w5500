/*!
 * \file helper.h
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/17 14:47:59 (KST)
 *
*/
#ifndef _HELPER_H_
#define _HELPER_H_

#include <stdint.h>
#include <string.h>
#include <wizchip_conf.h>

struct ip_port_rec {
	uint8_t ip[4];
	uint16_t port;
};

int gen_random_port(void);
void prn_ip_port(struct ip_port_rec * phost);
void pr_ip_addr(uint8_t addr[4]);
void pr_mac_addr(uint8_t addr[6]);
void pr_wiznetinfo(wiz_NetInfo * netinfo);


#define COPY_MAC_ADDR(dest, src) memcpy(dest, src, sizeof(uint8_t)*6)
#define COPY_IP_ADDR(dest, src) memcpy(dest, src, sizeof(uint8_t)*4)
#define SET_IP_PORT(dest, _ipval_, _portval_) do { COPY_IP_ADDR(dest.ip, _ipval_); dest.port = _portval_; } while(0)


#endif 
/********** end of file **********/
