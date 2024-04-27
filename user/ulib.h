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
int open(char *, int);
int close(int);
int read(int, char *, int);
int write(int, char *, int);
char getchar();

#endif