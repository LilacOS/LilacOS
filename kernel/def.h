#ifndef _DEF_H
#define _DEF_H

#include "types.h"

/* printf.c */
void printf(char *, ...);
void panic(char *, ...) __attribute__((noreturn));

/*  sbi.c    */
void console_putchar(usize c);
usize console_getchar();
void shutdown() __attribute__((noreturn));
void set_timer(usize time);

/*  trap.c */
void init_trap();

#endif