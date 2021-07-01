/*!
 * \file token.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : �� 4 04  Korea Standard Time 2008
 *
*/
#include "str_helper.h"
#include <string.h>

/// ���ڿ��� delimeter �� ������ �� ����ڰ� ���ϴ� ��ġ�� token �� ���´�
/// \param src ���� ���ڿ�
/// \param del delimeter ���ڿ� 
/// \param pos ���� ��ġ (1���� ����)
/// \param buf (out) token �� ����� buffer
/// \param buf_size buffer ũ�� 
/// \return position_pointer ���� ���ڿ����� token �� ��ġ
/// \return NULL token �� ���� �� ���� 
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
