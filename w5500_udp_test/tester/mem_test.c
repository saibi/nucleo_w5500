/*!
 * \file mem_test.c
 * \brief
 *
 *
 * created by Kim Youngmin (ymkim@huvitz.com)
 * creation date : 2021/06/22 18:06:26 (KST)
 *
*/
#include "tester.h"

#include <global_def.h>
#include <macro.h>
#include <wiz_appl.h>

#include <stdlib.h>
#include <string.h>

void malloc_test(void)
{
	extern char * syscall_heap_end;
	extern unsigned int syscall_heap_incr;
	extern char * syscall_start_of_heap_addr;
	extern char * syscall_end_of_heap_addr;

	extern uint8_t recv_buf[MAX_WIZ_BUF];

	int max = 0;
	char *alloc_p;
	int i;
	int size;

	DPN("&recv_buf = %p", recv_buf);
	DPN("&max= %p", &max);
	DPN("&size = %p", &size);

	DPN("syscall_start_of_heap_addr %p, syscall_end_of_heap_addr %p", syscall_start_of_heap_addr, syscall_end_of_heap_addr);

	size = sizeof(char) * 32;
	alloc_p = (char*)malloc(size);
	if ( alloc_p )
	{
		DPN("alloc_p = %p", alloc_p);

		strncpy(alloc_p, "hello malloc", size);

		DPN("[%s]", alloc_p);
		free(alloc_p);
	}

	size = 2 * 1024;
	for ( i = 512 * 1024; i > 0; i -= size )
	{
		alloc_p = (char*)malloc(i);
		if ( alloc_p )
		{
			memset(alloc_p, 0, size);
			free(alloc_p);

			if ( i > max ) 
				max = i;

			break;
		} 
	}
	DPN("syscall_heap_incr = %d, syscall_heap_end = %p", syscall_heap_incr, syscall_heap_end);

	DPN("max alloc %d bytes", max);
}
/********** end of file **********/
