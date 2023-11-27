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

int getpid()
{
    return sys_call(SYS_getpid, 0, 0, 0);
}

int fork()
{
    return sys_call(SYS_fork, 0, 0, 0);
}

int wait()
{
    return sys_call(SYS_wait, 0, 0, 0);
}