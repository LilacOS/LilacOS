#ifndef _DEF_H
#define _DEF_H

/* printf.c */
void printf(char *, ...);
void panic(char *, ...) __attribute__((noreturn));

/*  sbi.c    */
void console_putchar(usize);
usize console_getchar();
void shutdown() __attribute__((noreturn));

#endif