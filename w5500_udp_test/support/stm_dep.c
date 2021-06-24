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

void console_writeb(char *buf, int size)
{
	int write_size = 0;
	int remain_size = size;

	while ( remain_size > 0 )
	{
        	HAL_UART_Transmit(&huart3, (uint8_t *)&buf[write_size], remain_size, 50);
		write_size = remain_size - huart3.TxXferCount;
		remain_size = huart3.TxXferCount;
	}
}

/********** end of file **********/
