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

#include <errno.h>

extern int _write(int FD, char *buffer, int len);
extern UART_HandleTypeDef huart3; 

int _write(int FD, char *buffer, int len)
{
        HAL_UART_Transmit(&huart3, (uint8_t *) buffer, len, 50);
        return len;
}                      


extern void * _sbrk(int32_t incr);

// statck pointer : register char * stack_ptr asm ("sp")
#define _estack 0x20080000
#define _Min_Heap_Size 0x200 
#define _Min_Stack_Size 0x400 
 
char * syscall_end_of_heap_addr = (char *)(_estack - _Min_Heap_Size - _Min_Stack_Size);
char * syscall_start_of_heap_addr;
char * syscall_heap_end;
unsigned int syscall_heap_incr;

void * _sbrk(int32_t incr)
{
        extern char   end; /* Set by linker.  */
        char *        prev_heap_end;

        if (syscall_heap_end == 0) {
                syscall_start_of_heap_addr = syscall_heap_end = & end;
        }

        prev_heap_end = syscall_heap_end;

        if ( syscall_heap_end + incr > syscall_end_of_heap_addr )
        {
                _write(1, "_sbrk: Heap overflow\n", 22);
                syscall_heap_incr = incr;

                errno = ENOMEM;
                return (void *) -1;
        }
        syscall_heap_end += incr;

        return (void *) prev_heap_end;
}
/********** end of file **********/
