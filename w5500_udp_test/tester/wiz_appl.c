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

#include <socket.h>
#include <DHCP/dhcp.h>

#include <macro.h>
#include <helper.h>

#include <stdio.h>

static uint8_t tmp_wiz_buf[MAX_WIZ_BUF];
static uint8_t wiz_buf_size[] = { 4, 4, 8, 0, 0, 0, 0, 0 };

static void W5500_Select(void)
{
	HAL_GPIO_WritePin(GPIOB, W5500_CS_Pin, GPIO_PIN_RESET);
}

static void W5500_Deselect(void)
{
	HAL_GPIO_WritePin(GPIOB, W5500_CS_Pin, GPIO_PIN_SET);
}

static uint8_t W5500_ReadByte(void)
{
	unsigned char txByte = 0xff;	//dummy
	unsigned char rtnByte;
	while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
		;
	HAL_SPI_TransmitReceive(&hspi2, &txByte, &rtnByte, 1, 10);
	return rtnByte;
}

static void W5500_WriteByte(unsigned char txByte)
{
	unsigned char rtnByte;
	while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
		;
	HAL_SPI_TransmitReceive(&hspi2, &txByte, &rtnByte, 1, 10);
}

static void W5500_ReadBuff(uint8_t * buff, uint16_t len)
{

#ifdef SPI_POLLING
	while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
		;
	HAL_SPI_TransmitReceive(&hspi2, tmp_wiz_buf, buff, len, 10);
#else
	tmp_wiz_buf[0] = buff[0];
	tmp_wiz_buf[1] = buff[1];
	tmp_wiz_buf[2] = buff[2];
	HAL_SPI_TransmitReceive_DMA(&hspi2, tmp_wiz_buf, buff, len);
#endif
}

static void W5500_WriteBuff(uint8_t * buff, uint16_t len)
{
#ifdef SPI_POLLING
	while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
		;
	HAL_SPI_TransmitReceive(&hspi2, buff, tmp_wiz_buf, len, 10);
#else
	while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
		;
	HAL_SPI_TransmitReceive_DMA(&hspi2, buff, tmp_wiz_buf, len);
#endif
}

static void W5500_HardwareReset(void)
{
	HAL_GPIO_WritePin(GPIOF, W5500_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOF, W5500_RST_Pin, GPIO_PIN_SET);
}


void setup_wizchip(void)
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
int get_available_socket_no(void)
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
int get_DHCP_ip(uint8_t mac[6], wiz_NetInfo *netinfo)
{
	int sock;
	uint8_t buf[1024];
	int leased = 0;


	setSHAR(mac);

	sock = get_available_socket_no();
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
int get_socket_buf_size(int sock)
{
	if ( sock < 0 || sock > MAX_WIZ_SOCKET ) 
		return -1;

	return wiz_buf_size[sock] * 1024;
}

void HAL_SYSTICK_Callback(void)
{
	extern void DHCP_time_handler(void);
	static uint32_t prev_tick = 0;

	if ( (uwTick - prev_tick) >= TICKS_PER_SECOND ) 
	{
		DHCP_time_handler();   //1sec마다 증가...
		prev_tick = uwTick;
	}
}

/********** end of file **********/
