#ifndef _DEF_H
#define _DEF_H

#include "types.h"

/*  sbi.c    */
void console_putchar(usize c);
usize console_getchar();
void shutdown() __attribute__((noreturn));
void set_timer(usize time);

/* printf.c */
void printf(char *, ...);
void panic(char*) __attribute__((noreturn));

/*  interrupt.c */
void init_interrupt();

#endif