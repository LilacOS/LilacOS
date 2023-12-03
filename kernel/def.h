#ifndef _DEF_H
#define _DEF_H

#include "types.h"

/* kerneltrap.S */
void __trap_entry();

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

/* timer.c */
void init_timer();
void set_next_timeout();

#endif