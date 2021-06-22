/*!
 * \file helper.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/05/17 13:31:06 (KST)
 *
*/
#include "helper.h"
#include <macro.h>

#include <stdlib.h>
#include <stdio.h>

/// random port number generator
/// \return port number (> 2000)
int gen_random_port(void)
{
	static int need_srand = 1;

	if ( need_srand )
	{
		srand(CURRENT_TICK);
		need_srand = 0;
	}

	return (rand() % 30000) + 2000;
}

/// print ip address & port number
void prn_ip_port(uint8_t *ip, uint16_t port)
{
	printf("%d.%d.%d.%d:%d\r\n", ip[0], ip[1], ip[2], ip[3], port);
}

/// print ip v4 address
void pr_ip4_addr(uint8_t addr[4])
{
	printf("%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
}

/// print mac address
void pr_mac_addr(uint8_t addr[6])
{
	printf("%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[4]);
}

/// print wiz_NetInfo structure
void pr_wiznetinfo(wiz_NetInfo * netinfo)
{
	printf("mac: ");
	pr_mac_addr(netinfo->mac);
	printf("\r\nip: ");
	pr_ip4_addr(netinfo->ip);
	printf("\r\ngateway: ");
	pr_ip4_addr(netinfo->gw);
	printf("\r\nnetmask: ");
	pr_ip4_addr(netinfo->sn);
	printf("\r\ndns: ");
	pr_ip4_addr(netinfo->dns);
	printf("\r\n");
}
/********** end of file **********/
