/*!
 * \file macro.h
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/05/13 14:17:49 (KST)
 *
*/
#ifndef _MACRO_H_
#define _MACRO_H_

#include <stdio.h>

#include "tick.h"

#define DPF(fmt, args...) printf("%d:%s:%d:%s() " fmt, CURRENT_TICK, __FILE__, __LINE__, __func__, ## args)
#define DP(FMT, args...) printf("%d:%s() " FMT, CURRENT_TICK, __func__, ## args)
#define DPN(fmt, args...) DP(fmt "\r\n", ## args)

#endif 
/********** end of file **********/
