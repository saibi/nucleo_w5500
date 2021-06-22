/*!
 * \file syscalls.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/17 14:22:56 (KST)
 *
*/
#include <stm32f7xx_hal.h>

extern int _write(int FD, char *buffer, int len);
extern UART_HandleTypeDef huart3; 

int _write(int FD, char *buffer, int len)
{
        HAL_UART_Transmit(&huart3, (uint8_t *) buffer, len, 50);
        return len;
}                      
/********** end of file **********/
