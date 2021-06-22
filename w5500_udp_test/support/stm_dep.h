/*!
 * \file stm_dep.h
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/17 13:47:23 (KST)
 *
*/
#ifndef _STM_DEP_H_
#define _STM_DEP_H_

//
// stm32 header & macros 
//
#include <stm32f7xx_hal.h>                                                                   
#include <stdint.h>

extern __IO uint32_t uwTick;

extern SPI_HandleTypeDef hspi2;



#endif
/********** end of file **********/
