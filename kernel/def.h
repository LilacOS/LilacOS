#ifndef _DEF_H
#define _DEF_H

#include "types.h"

struct Buddy;

/* buddy_system_allocator.c */
void init_buddy(struct Buddy *buddy);
void add_to_buddy(struct Buddy *buddy, void *start, void *end);
void *buddy_alloc(struct Buddy *buddy, uint64 size);
void buddy_dealloc(struct Buddy *buddy, void *block, uint64 size);

/* kerneltrap.S */
void __trap_entry();

/*  memory.c    */
void init_memory();
void *alloc(usize size);
void dealloc(void *block, usize size);
usize alloc_frame();
void dealloc_frame(usize ppn);

/* mapping.c */
void map_kernel();

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