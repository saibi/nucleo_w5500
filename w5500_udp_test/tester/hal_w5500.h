/*!
 * \file hal_w5500.h
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/24 09:22:45 (KST)
 *
*/
#ifndef _HAL_W5500_H_
#define _HAL_W5500_H_

#include <stdint.h>

void W5500_Select(void);
void W5500_Deselect(void);
uint8_t W5500_ReadByte(void);
void W5500_WriteByte(unsigned char txByte);
void W5500_WriteByte(unsigned char txByte);
void W5500_ReadBuff(uint8_t * buff, uint16_t len);
void W5500_WriteBuff(uint8_t * buff, uint16_t len);
void W5500_HardwareReset(void);
void HAL_SYSTICK_Callback(void);

#endif 
/********** end of file **********/
