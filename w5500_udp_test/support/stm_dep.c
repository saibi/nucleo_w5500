/*!
 * \file stm_dep.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/24 10:38:56 (KST)
 *
*/
#include "stm_dep.h"

#define MAX_TRANSFER 65535
#define WRITE_TPMS (115200/8/2/1000)

void console_writeb(char *buf, int size)
{
	int wait = 50;
	int write_size = 0;
	int remain_size = size;
	int transferred = 0;
	uint16_t transfer;
	
	while ( remain_size > 0 )
	{
		if ( remain_size > MAX_TRANSFER )
			transfer = MAX_TRANSFER;
		else
			transfer = remain_size;

		if ( transfer > WRITE_TPMS * wait )
			wait = (transfer / WRITE_TPMS + 1);

        	while (HAL_BUSY == HAL_UART_Transmit(&huart3, (uint8_t *)&buf[write_size], transfer, wait) ) 
			;

		transferred = transfer - huart3.TxXferCount;
		write_size += transferred;
		remain_size -= transferred;
	}
}

/********** end of file **********/
