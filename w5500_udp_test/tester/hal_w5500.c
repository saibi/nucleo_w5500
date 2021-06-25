/*!
 * \file hal_w5500.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/24 09:19:02 (KST)
 *
*/
#include "hal_w5500.h"
#include <global_def.h>
#include <tick.h>

uint8_t w5500_dummy_buf[MAX_WIZ_BUF];

void W5500_Select(void)
{
	HAL_GPIO_WritePin(GPIOB, W5500_CS_Pin, GPIO_PIN_RESET);
}

void W5500_Deselect(void)
{
	HAL_GPIO_WritePin(GPIOB, W5500_CS_Pin, GPIO_PIN_SET);
}

uint8_t W5500_ReadByte(void)
{
	unsigned char txByte = 0xff;	//dummy
	unsigned char rtnByte;
	while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
		;
	HAL_SPI_TransmitReceive(&hspi2, &txByte, &rtnByte, 1, 10);
	return rtnByte;
}

void W5500_WriteByte(unsigned char txByte)
{
	unsigned char rtnByte;
	while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
		;
	HAL_SPI_TransmitReceive(&hspi2, &txByte, &rtnByte, 1, 10);
}

void W5500_ReadBuff(uint8_t * buff, uint16_t len)
{

#ifdef SPI_POLLING
	while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
		;
	HAL_SPI_TransmitReceive(&hspi2, w5500_dummy_buf, buff, len, 10);
#else
	w5500_dummy_buf[0] = buff[0];
	w5500_dummy_buf[1] = buff[1];
	w5500_dummy_buf[2] = buff[2];
	HAL_SPI_TransmitReceive_DMA(&hspi2, w5500_dummy_buf, buff, len);
#endif
}

void W5500_WriteBuff(uint8_t * buff, uint16_t len)
{
#ifdef SPI_POLLING
	while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
		;
	HAL_SPI_TransmitReceive(&hspi2, buff, w5500_dummy_buf, len, 10);
#else
	while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
		;
	HAL_SPI_TransmitReceive_DMA(&hspi2, buff, w5500_dummy_buf, len);
#endif
}

void W5500_HardwareReset(void)
{
	HAL_GPIO_WritePin(GPIOF, W5500_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOF, W5500_RST_Pin, GPIO_PIN_SET);
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
