#include "syscall.h"
#include "types.h"

ssize_t write(int fd, const void *buf, size_t count)
{
    sys_call(Write, fd, buf, count);
}

void exit(int status)
{
    sys_call(Exit_group, status, 0, 0);
}