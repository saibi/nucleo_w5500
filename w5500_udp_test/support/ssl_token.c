/*!
 * \file token.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 금 4 04  Korea Standard Time 2008
 *
*/
#include "str_helper.h"
#include <string.h>

/// 문자열을 delimeter 로 구분한 후 사용자가 원하는 위치의 token 을 얻어온다
/// \param src 원본 문자열
/// \param del delimeter 문자열 
/// \param pos 얻어올 위치 (1부터 시작)
/// \param buf (out) token 이 저장될 buffer
/// \param buf_size buffer 크기 
/// \return position_pointer 원본 문자열에서 token 의 위치
/// \return NULL token 을 얻을 수 없음 
char * extract_token(char *src, char *del, int pos, char *buf, int buf_size)
{
	char *token_start = src;
	char *token_end = src;
	int i;
	int del_len = strlen(del);
	int token_len;
	
	for ( i = 0 ; i < pos ; i ++ ) {
		if ( (token_end = strstr( token_end, del )) != NULL) {
			if ( i == pos - 1 ) {
				token_len = (int)(token_end - token_start);
				
				if ( token_len > buf_size ) {
					strncpy(buf, token_start, buf_size);
					buf[buf_size -1] = 0;
				} else {
					strncpy(buf, token_start,(int)(token_end - token_start));
				}
				buf[token_len] = 0;
				return token_start;
			} else {
				token_start = (char *)(token_end + del_len);
				token_end += del_len;
			}
		} else {
			if ( i == pos - 1 ) {
				strncpy(buf, token_start, buf_size);
				buf[buf_size-1] = 0;
				return token_start;
			} else {
				return NULL;
			}
		}
	}
	return NULL;
}

/********** end of file **********/
