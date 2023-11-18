#ifndef _SYSCALL_H
#define _SYSCALL_H

#define SYS_exit 0
#define SYS_putchar 1

#define sys_call(__num, __a0, __a1, __a2)                             \
    ({                                                                \
        register unsigned long a0 asm("a0") = (unsigned long)(__a0);  \
        register unsigned long a1 asm("a1") = (unsigned long)(__a1);  \
        register unsigned long a2 asm("a2") = (unsigned long)(__a2);  \
        register unsigned long a7 asm("a7") = (unsigned long)(__num); \
        asm volatile("ecall"                                          \
                     : "+r"(a0)                                       \
                     : "r"(a1), "r"(a2), "r"(a7)                      \
                     : "memory");                                     \
        a0;                                                           \
    })

#endif