#include "kernel/syscall.h"
#include "kernel/types.h"

#define sys_call(__num, __a0, __a1, __a2)                                      \
    ({                                                                         \
        register unsigned long a0 asm("a0") = (unsigned long)(__a0);           \
        register unsigned long a1 asm("a1") = (unsigned long)(__a1);           \
        register unsigned long a2 asm("a2") = (unsigned long)(__a2);           \
        register unsigned long a7 asm("a7") = (unsigned long)(__num);          \
        asm volatile("ecall"                                                   \
                     : "+r"(a0)                                                \
                     : "r"(a1), "r"(a2), "r"(a7)                               \
                     : "memory");                                              \
        a0;                                                                    \
    })

void exit() { sys_call(SYS_exit, 0, 0, 0); }

void putchar(char c) { sys_call(SYS_putchar, c, 0, 0); }

int getpid() { return sys_call(SYS_getpid, 0, 0, 0); }

int fork() { return sys_call(SYS_fork, 0, 0, 0); }

int wait() { return sys_call(SYS_wait, 0, 0, 0); }