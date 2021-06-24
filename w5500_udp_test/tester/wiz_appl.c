/*!
 * \file wiz_appl.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/18 10:20:21 (KST)
 *
*/
#include "wiz_appl.h"
#include <hal_w5500.h>

#include <socket.h>
#include <DHCP/dhcp.h>

#include <macro.h>
#include <helper.h>

#include <stdio.h>

static uint8_t wiz_buf_size[] = { 2, 2, 8, 0, 0, 0, 0, 0 };

void wiz_init_chip(void)
{
	int ret;

	W5500_HardwareReset();
	reg_wizchip_cs_cbfunc(W5500_Select, W5500_Deselect);
	reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);
	reg_wizchip_spiburst_cbfunc(W5500_ReadBuff, W5500_WriteBuff);

	ret = wizchip_init(wiz_buf_size, wiz_buf_size);
	DPN("setup wiz chip 5500. %d", ret);
}

/// return unused socket number
/// \return socket number 0~7
/// \return -1 socket unavialable
int wiz_get_available_socket_no(void)
{
	int i;

	// w5500 max : _WIZCHIP_SOCK_NUM_ 
	
	for (i = 0 ; i < MAX_WIZ_SOCKET ; ++i )
	{
		if ( getSn_SR(i) == 0 ) 
			return i;
	}
	return -1;
}


/// get dhcp ip
/// \param mac (in) mac address
/// \param netinfo (out) dhcp address 
/// \return 0 success
/// \return -1 error or failed
int wiz_get_dhcp_ip(uint8_t mac[6], wiz_NetInfo *netinfo)
{
	int sock;
	uint8_t buf[1024];
	int leased = 0;


	setSHAR(mac);

	sock = wiz_get_available_socket_no();
	if ( sock < 0 ) 
	{
		DPN("socket unavialable.");
		return -1;
	}

	DHCP_init(sock, buf);

	TIME_LOOP_START(5)
		switch( DHCP_run() )
		{
		//case DHCP_IP_ASSIGN:
		//case DHCP_IP_CHANGED:
		
		case DHCP_IP_LEASED:
			leased = 1;
			break;

		case DHCP_FAILED:
			DPN("DHCP failed");
			break;
		default:
			break;

		}
		HAL_Delay(100);
	TIME_LOOP_END

	DHCP_stop();

	if ( !leased ) 
		return -1;

	getIPfromDHCP(netinfo->ip);
	getGWfromDHCP(netinfo->gw);
	getSNfromDHCP(netinfo->sn);
	getDNSfromDHCP(netinfo->dns);
	netinfo->dhcp = NETINFO_DHCP;
	COPY_MAC_ADDR(netinfo->mac, mac);

	return 0;
}


/// return socket rx/tx buf size
int wiz_get_socket_buf_size(int sock)
{
	if ( sock < 0 || sock > MAX_WIZ_SOCKET ) 
		return -1;

	return wiz_buf_size[sock] * 1024;
}

/// block until (size) bytes sent
int wiz_sendb(int sock, char *buf, int size)
{
	int ret = 0;
	int sent_size = 0;
	int remain_size = size;

	do {
		ret = send(sock, (uint8_t*)&buf[sent_size], remain_size);
		if ( ret < 0 ) 
		{
			DPN("send error = %d", ret);
			if ( sent_size > 0 )
				DPN("%d bytes sent", sent_size);

			return ret;
		}
		remain_size -= ret;
		sent_size += ret;
	} while( remain_size > 0 );

	return sent_size;
}

/// block until (size) bytes received 
int wiz_recvb(int sock, char *buf, int size)
{
	int ret = 0;
	int recv_size = 0;
	int remain_size = size;

	do {
		ret = recv(sock, (uint8_t*)&buf[recv_size], remain_size);
		if ( ret < 0 ) 
		{
			DPN("recv error = %d", ret);
			if ( recv_size > 0 )
				DPN("%d bytes received", recv_size);

			return ret;
		}
		remain_size -= ret;
		recv_size += ret;
	} while ( remain_size > 0 );

	return recv_size;
}

/// \param (in) netinfo->mac 
/// \param (out) netinfo 
void wiz_set_dhcp_ip(wiz_NetInfo *netinfo)
{
	DPN("get DHCP ip ...");
	if ( wiz_get_dhcp_ip(netinfo->mac, netinfo) < 0 ) 
		DPN("ip error\n");
	else 
		pr_wiznetinfo(netinfo);

	wizchip_setnetinfo(netinfo);
}


/********** end of file **********/
