#ifndef _ULIB_H
#define _ULIB_H

/* printf.c */
void printf(char *, ...);
void panic(char *);

/* syscall.c */
void putchar(char c);
void exit();

#endif