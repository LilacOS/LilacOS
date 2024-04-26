#include "types.h"
#include "def.h"
#include "syscall.h"
#include "process.h"

extern struct ProcessControlBlock *current;

usize syscall(usize id, usize args[3]) {
    switch (id) {
    case SYS_exit:
        exit_current();
    case SYS_putchar:
        console_putchar(args[0]);
        return 0;
    case SYS_getpid:
        return current->pid;
    case SYS_fork:
        return sys_fork();
    case SYS_wait:
        return sys_wait();
    case SYS_exec:
        return sys_exec((char *)args[0]);
    case SYS_open:
        return sys_open((char *)args[0], args[1]);
    case SYS_close:
        return sys_close(args[0]);
    case SYS_read:
    case SYS_write:
    default:
        panic("[syscall] Unknown syscall id %d\n", id);
    }
}