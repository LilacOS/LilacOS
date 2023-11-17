#ifndef _ULIB_H
#define _ULIB_H

/* printf.c */
void printf(char *, ...);
void panic(char *);
void putchar(int c);

/* syscall.c */
ssize_t write(int fd, const void *buf, size_t count);
void exit(int status);

#endif