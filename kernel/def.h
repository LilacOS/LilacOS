#ifndef _DEF_H
#define _DEF_H

#include "types.h"

struct Buddy;
struct Segment;
struct Mapping;
struct TrapContext;
struct TaskContext;

/* buddy_system_allocator.c */
void init_buddy(struct Buddy *);
void add_to_buddy(struct Buddy *, void *, void *);
void *buddy_alloc(struct Buddy *, uint64);
void buddy_dealloc(struct Buddy *, void *, uint64);

/* elf.c */
struct Mapping new_user_mapping(char *);

/* kerneltrap.S */
void __trap_entry();
void __restore(struct TrapContext *);

/*  memory.c    */
void init_memory();
void *alloc(usize);
void dealloc(void *, usize);
usize alloc_frame();
void dealloc_frame(usize);

/* mapping.c */
void map_kernel();
struct Mapping new_kernel_mapping();
void map_framed_segment(struct Mapping, struct Segment, char *, usize);

/* printf.c */
void printf(char *, ...);
void panic(char *, ...) __attribute__((noreturn));

/*  sbi.c    */
void console_putchar(usize);
usize console_getchar();
void shutdown() __attribute__((noreturn));
void set_timer(usize);

/* syscall.c */
usize syscall(usize, usize[3], struct TrapContext *);

/* switch.S */
void __switch(struct TaskContext *, struct TaskContext *);

/*  trap.c */
void init_trap();

/* task.c */
void schedule();
void init_task();
void exit_current();

/* timer.c */
void init_timer();
void set_next_timeout();

#endif