//*****************************************************************************
//
//! \file w5500.c
//! \brief W5500 HAL Interface.
//! \version 1.0.2
//! \date 2013/10/21
//! \par  Revision history
//!       <2015/02/05> Notice
//!        The version history is not updated after this point.
//!        Download the latest version directly from GitHub. Please visit the our GitHub repository for ioLibrary.
//!        >> https://github.com/Wiznet/ioLibrary_Driver
//!       <2014/05/01> V1.0.2
//!         1. Implicit type casting -> Explicit type casting. Refer to M20140501
//!            Fixed the problem on porting into under 32bit MCU
//!            Issued by Mathias ClauBen, wizwiki forum ID Think01 and bobh
//!            Thank for your interesting and serious advices.
//!       <2013/12/20> V1.0.1
//!         1. Remove warning
//!         2. WIZCHIP_READ_BUF WIZCHIP_WRITE_BUF in case _WIZCHIP_IO_MODE_SPI_FDM_
//!            for loop optimized(removed). refer to M20131220
//!       <2013/10/21> 1st Release
//! \author MidnightCow
//! \copyright
//!
//! Copyright (c)  2013, WIZnet Co., LTD.
//! All rights reserved.
//! 
//! Redistribution and use in source and binary forms, with or without 
//! modification, are permitted provided that the following conditions 
//! are met: 
//! 
//!     * Redistributions of source code must retain the above copyright 
//! notice, this list of conditions and the following disclaimer. 
//!     * Redistributions in binary form must reproduce the above copyright
//! notice, this list of conditions and the following disclaimer in the
//! documentation and/or other materials provided with the distribution. 
//!     * Neither the name of the <ORGANIZATION> nor the names of its 
//! contributors may be used to endorse or promote products derived 
//! from this software without specific prior written permission. 
//! 
//! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//! AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
//! IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//! ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
//! LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
//! CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
//! SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//! INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
//! CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
//! ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
//! THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************
//#include <stdio.h>
#include "stm32f7xx_hal.h"

#include "w5500.h"

#include <string.h>

#define MAX_WIZ_BUF (8*1024 + 4) // make sure MAX_WIZ_BUF valure is bigger than wizchip_init() size 

extern SPI_HandleTypeDef hspi2;
uint8_t w5500_Tx_Buf[MAX_WIZ_BUF]; 
uint8_t w5500_Rx_Buf[MAX_WIZ_BUF];
extern volatile uint8_t spi2_cs;

#define _W5500_SPI_VDM_OP_          0x00
#define _W5500_SPI_FDM_OP_LEN1_     0x01
#define _W5500_SPI_FDM_OP_LEN2_     0x02
#define _W5500_SPI_FDM_OP_LEN4_     0x03

#if   (_WIZCHIP_ == 5500)
////////////////////////////////////////////////////

uint8_t  WIZCHIP_READ(uint32_t AddrSel)
{
   uint8_t ret = 0;
   //uint8_t spi_data[3];

   WIZCHIP_CRITICAL_ENTER();
   spi2_cs = 0;
   //while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY);

   WIZCHIP.CS._select();
   //for(int i=0; i<100;i++);

   AddrSel |= (_W5500_SPI_READ_ | _W5500_SPI_VDM_OP_);

   if(!WIZCHIP.IF.SPI._read_burst || !WIZCHIP.IF.SPI._write_burst) 	// byte operation
   {
	    WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
   }
   else																// burst operation
   {
		//spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
		//spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
		//spi_data[2] = (AddrSel & 0x000000FF) >> 0;
		//WIZCHIP.IF.SPI._write_burst(spi_data, 3);
#ifdef SPI_POLLING
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
		ret = WIZCHIP.IF.SPI._read_byte();

		//printf("AddrSel = %06x rx=%02x ,",AddrSel, ret);
#else
		w5500_Rx_Buf[0] = (AddrSel & 0x00FF0000) >> 16;
		w5500_Rx_Buf[1] = (AddrSel & 0x0000FF00) >> 8;
		w5500_Rx_Buf[2] = (AddrSel & 0x000000FF) >> 0;

		WIZCHIP.IF.SPI._read_burst(w5500_Rx_Buf, 4);


		while(spi2_cs < 2);
		//HAL_GPIO_WritePin(GPIOB, LD2_Pin,GPIO_PIN_SET);
		//HAL_GPIO_WritePin(GPIOB, LD2_Pin,GPIO_PIN_RESET);

		ret = w5500_Rx_Buf[3];
		//printf("AddrSel1 = %06x rx=%02x\r\n,",AddrSel, ret);

		///ret = w5500_Rx_Buf[0];
		//printf("AddrSel2 = %06x rx=%02x ,",AddrSel, ret);
		//ret = w5500_Rx_Buf[0];
		//printf("AddrSel3 = %06x rx=%02x ,",AddrSel, ret);

#endif

   }
   //ret = WIZCHIP.IF.SPI._read_byte();

   //printf("call WIZCHIP_Read burst\r\n");

   WIZCHIP.CS._deselect();
   WIZCHIP_CRITICAL_EXIT();
   return ret;
}

void     WIZCHIP_WRITE(uint32_t AddrSel, uint8_t wb )
{
   //uint8_t spi_data[4];

   WIZCHIP_CRITICAL_ENTER();
   spi2_cs = 0;
   //while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY);
   WIZCHIP.CS._select();
   //for(int i=0; i<100;i++);

   AddrSel |= (_W5500_SPI_WRITE_ | _W5500_SPI_VDM_OP_);

   //if(!WIZCHIP.IF.SPI._read_burst || !WIZCHIP.IF.SPI._write_burst) 	// byte operation
   if(!WIZCHIP.IF.SPI._write_burst) 	// byte operation
   {
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
		WIZCHIP.IF.SPI._write_byte(wb);
   }
   else									// burst operation
   {
		//spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
		//spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
		//spi_data[2] = (AddrSel & 0x000000FF) >> 0;
		//spi_data[3] = wb;
		//WIZCHIP.IF.SPI._write_burst(spi_data, 4);
#ifdef  SPI_POLLING
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
		WIZCHIP.IF.SPI._write_byte(wb);

		 //printf("AddrSel = %06x tx=%02x ,",AddrSel, wb);
#else


		w5500_Tx_Buf[0] = (AddrSel & 0x00FF0000) >> 16;
		w5500_Tx_Buf[1] = (AddrSel & 0x0000FF00) >> 8;
		w5500_Tx_Buf[2] = (AddrSel & 0x000000FF) >> 0;
		w5500_Tx_Buf[3] = wb;

		//printf("Tx=%02x %02x %02x %02x \r\n",w5500_Tx_Buf[0],w5500_Tx_Buf[1],w5500_Tx_Buf[2],w5500_Tx_Buf[3]);
		WIZCHIP.IF.SPI._write_burst(w5500_Tx_Buf, 4);
		while(spi2_cs < 2);
		//HAL_GPIO_WritePin(GPIOB, LD2_Pin,GPIO_PIN_SET);
		//HAL_GPIO_WritePin(GPIOB, LD2_Pin,GPIO_PIN_RESET);
		//printf("AddrSel = %06x tx=%02x ,",AddrSel, wb);


#endif

   }
   //printf("call WIZCHIP_WRITE burst\r\n");

   WIZCHIP.CS._deselect();
   WIZCHIP_CRITICAL_EXIT();
}
         
void     WIZCHIP_READ_BUF (uint32_t AddrSel, uint8_t* pBuf, uint16_t len)
{
   //uint8_t spi_data[3];
   uint16_t i;

   WIZCHIP_CRITICAL_ENTER();
   spi2_cs = 0;
   //while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY);
   //while(HAL_SPI_GetState(&hspi2) == HAL_SPI_STATE_BUSY_TX);
   /*
   while(1)
   {
	   if(!spi2_cs)   HAL_GPIO_WritePin(GPIOB, LD2_Pin, GPIO_PIN_SET);
	   else
	   {
		   HAL_GPIO_WritePin(GPIOB, LD2_Pin, GPIO_PIN_RESET);
		   break;
	   }
   }
   */
   WIZCHIP.CS._select();

   AddrSel |= (_W5500_SPI_READ_ | _W5500_SPI_VDM_OP_);

   if(!WIZCHIP.IF.SPI._read_burst || !WIZCHIP.IF.SPI._write_burst) 	// byte operation
   {
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
		for(i = 0; i < len; i++)
		   pBuf[i] = WIZCHIP.IF.SPI._read_byte();
   }
   else																// burst operation
   {

#ifdef SPI_POLLING
		spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
		spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
		spi_data[2] = (AddrSel & 0x000000FF) >> 0;
		WIZCHIP.IF.SPI._write_burst(spi_data, 3);
		WIZCHIP.IF.SPI._read_burst(pBuf, len);


#else
		//gTx_Buf[0] = (AddrSel & 0x00FF0000) >> 16;
		//gTx_Buf[1] = (AddrSel & 0x0000FF00) >> 8;
		//gTx_Buf[2] = (AddrSel & 0x000000FF) >> 0;
		//WIZCHIP.IF.SPI._write_burst(gTx_Buf, 3);
		//WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
		//WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
		//WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);

		/*
		pBuf[0] = (AddrSel & 0x00FF0000) >> 16;
		pBuf[1] = (AddrSel & 0x0000FF00) >> 8;
		pBuf[2] = (AddrSel & 0x000000FF) >> 0;

		WIZCHIP.IF.SPI._read_burst(pBuf, len+3);
		*/
		w5500_Rx_Buf[0] = (AddrSel & 0x00FF0000) >> 16;
		w5500_Rx_Buf[1] = (AddrSel & 0x0000FF00) >> 8;
		w5500_Rx_Buf[2] = (AddrSel & 0x000000FF) >> 0;

		WIZCHIP.IF.SPI._read_burst(w5500_Rx_Buf, len+3);
		while(spi2_cs < 2);
		//HAL_GPIO_WritePin(GPIOB, LD2_Pin,GPIO_PIN_SET);
		//HAL_GPIO_WritePin(GPIOB, LD2_Pin,GPIO_PIN_RESET);

		//printf("Rx1 = %02x:%02x:%02x:%02x:%02x:%02x\r\n",w5500_Rx_Buf[3],w5500_Rx_Buf[4],w5500_Rx_Buf[5],w5500_Rx_Buf[6],w5500_Rx_Buf[7],w5500_Rx_Buf[8]);

		memcpy((uint8_t *)pBuf, (uint8_t *)(w5500_Rx_Buf+3), len);

		//printf("Rx2 = %02x:%02x:%02x:%02x:%02x:%02x\r\n",pBuf[0],pBuf[1],pBuf[2],pBuf[3],pBuf[4],pBuf[5]);


#endif
   }
   WIZCHIP.CS._deselect();
   WIZCHIP_CRITICAL_EXIT();
}

void     WIZCHIP_WRITE_BUF(uint32_t AddrSel, uint8_t* pBuf, uint16_t len)
{
   //uint8_t spi_data[3];
   uint16_t i;

   WIZCHIP_CRITICAL_ENTER();
   //while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY);
   //printf("Select \r\n");
   WIZCHIP.CS._select();
   spi2_cs = 0;

   AddrSel |= (_W5500_SPI_WRITE_ | _W5500_SPI_VDM_OP_);

   if(!WIZCHIP.IF.SPI._write_burst) 	// byte operation
   {
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
		WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
		for(i = 0; i < len; i++)
			WIZCHIP.IF.SPI._write_byte(pBuf[i]);
   }
   else									// burst operation
   {

		//WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
		//WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
		//WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);

		//HAL_SPI_TransmitReceive(&hspi2,spi_data,gTemp,3,10);
		//
		// while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY);
		//for(i = 0; i < 3; i++)
		//	WIZCHIP.IF.SPI._write_byte(spi_data[i]);
		//HAL_SPI_TransmitReceive_DMA(&hspi2,spi_data,gTemp,3);
		//while(HAL_SPI_GetState(&hspi2) == HAL_SPI_STATE_BUSY_TX);
		// while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY);
		//HAL_SPI_TransmitReceive_DMA(&hspi2,pBuf,gTemp,len);
		//while(HAL_SPI_GetState(&hspi2) == HAL_SPI_STATE_BUSY_TX);

		//WIZCHIP.IF.SPI._write_burst(spi_data, 3);
		//WIZCHIP.IF.SPI._write_burst(pBuf, len);
#ifdef SPI_POLLING
	   spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
	   spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
	   spi_data[2] = (AddrSel & 0x000000FF) >> 0;
	   WIZCHIP.IF.SPI._write_burst(spi_data, 3);
	   WIZCHIP.IF.SPI._write_burst(pBuf, len);


#else
	    //printf("Select memcpy %08x len=%d \r\n",pBuf,len);
		memcpy((uint8_t *)(w5500_Tx_Buf+3), (uint8_t *)pBuf, len);

		w5500_Tx_Buf[0] = (AddrSel & 0x00FF0000) >> 16;
		w5500_Tx_Buf[1] = (AddrSel & 0x0000FF00) >> 8;
		w5500_Tx_Buf[2] = (AddrSel & 0x000000FF) >> 0;

		//memcpy((uint8_t *)(pBuf), (uint8_t *)w5500_Tx_Buf, len+3);
		//printf("Select Middle\r\n");
		WIZCHIP.IF.SPI._write_burst(w5500_Tx_Buf, len+3);
		while(spi2_cs < 2);
		//HAL_GPIO_WritePin(GPIOB, LD2_Pin,GPIO_PIN_SET);
		//HAL_GPIO_WritePin(GPIOB, LD2_Pin,GPIO_PIN_RESET);


#endif
   }
   //printf("DeSelect \r\n");
   WIZCHIP.CS._deselect();
   WIZCHIP_CRITICAL_EXIT();
}


uint16_t getSn_TX_FSR(uint8_t sn)
{
   uint16_t val=0,val1=0;

   do
   {
      val1 = WIZCHIP_READ(Sn_TX_FSR(sn));
      val1 = (val1 << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_TX_FSR(sn),1));
      if (val1 != 0)
      {
        val = WIZCHIP_READ(Sn_TX_FSR(sn));
        val = (val << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_TX_FSR(sn),1));
      }
   }while (val != val1);
   return val;
}


uint16_t getSn_RX_RSR(uint8_t sn)
{
   uint16_t val=0,val1=0;

   do
   {
      val1 = WIZCHIP_READ(Sn_RX_RSR(sn));
      val1 = (val1 << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_RX_RSR(sn),1));
      if (val1 != 0)
      {
        val = WIZCHIP_READ(Sn_RX_RSR(sn));
        val = (val << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_RX_RSR(sn),1));
      }
   }while (val != val1);
   return val;
}

void wiz_send_data(uint8_t sn, uint8_t *wizdata, uint16_t len)
{
   uint16_t ptr = 0;
   uint32_t addrsel = 0;

   //printf("wiz_send_data start\r\n");

   if(len == 0)  return;
   ptr = getSn_TX_WR(sn);
   //M20140501 : implict type casting -> explict type casting
   //addrsel = (ptr << 8) + (WIZCHIP_TXBUF_BLOCK(sn) << 3);
   addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_TXBUF_BLOCK(sn) << 3);
   //
   //printf("wiz_send_data middle \r\n");
   WIZCHIP_WRITE_BUF(addrsel,wizdata, len);
   
   ptr += len;
   setSn_TX_WR(sn,ptr);

   //printf("wiz_send_data end\r\n");
}

void wiz_recv_data(uint8_t sn, uint8_t *wizdata, uint16_t len)
{
   uint16_t ptr = 0;
   uint32_t addrsel = 0;
   
   if(len == 0) return;
   ptr = getSn_RX_RD(sn);
   //M20140501 : implict type casting -> explict type casting
   //addrsel = ((ptr << 8) + (WIZCHIP_RXBUF_BLOCK(sn) << 3);
   addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_RXBUF_BLOCK(sn) << 3);
   //
   WIZCHIP_READ_BUF(addrsel, wizdata, len);
   ptr += len;
   
   setSn_RX_RD(sn,ptr);
}


void wiz_recv_ignore(uint8_t sn, uint16_t len)
{
   uint16_t ptr = 0;

   ptr = getSn_RX_RD(sn);
   ptr += len;
   setSn_RX_RD(sn,ptr);
}

#endif
