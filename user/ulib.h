#ifndef _ULIB_H
#define _ULIB_H

/* printf.c */
void printf(char *, ...);
void panic(char *, ...);

/* syscall.c */
void putchar(char);
void exit();
int getpid();
int fork();
int wait();
int exec(char *);

#endif