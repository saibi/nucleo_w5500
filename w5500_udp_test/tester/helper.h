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
#include <wizchip_conf.h>

int gen_random_port(void);
void prn_ip_port(uint8_t *ip, uint16_t port);
void pr_ip4_addr(uint8_t addr[4]);
void pr_mac_addr(uint8_t addr[6]);
void pr_wiznetinfo(wiz_NetInfo * netinfo);

#endif 
/********** end of file **********/
