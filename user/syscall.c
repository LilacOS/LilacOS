#include "syscall.h"
#include "types.h"

void exit()
{
    sys_call(SYS_exit, 0, 0, 0);
}

void putchar(char c)
{
    sys_call(SYS_putchar, c, 0, 0);
}