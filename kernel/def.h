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
void panic(char *) __attribute__((noreturn));

/*  trap.c */
void init_trap();

/*  memory.c    */
void init_memory();
void *alloc(usize size);
void dealloc(void *block, usize size);
usize alloc_frame();
void dealloc_frame(usize ppn);

#endif