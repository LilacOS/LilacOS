#ifndef _DEF_H
#define _DEF_H

#include "types.h"

struct Buddy;
struct Segment;
struct MemoryMap;
struct TrapContext;
struct TaskContext;
struct Task;
struct Inode;
enum SegmentType;

/* buddy_system_allocator.c */
void init_buddy(struct Buddy *);
void add_to_buddy(struct Buddy *, void *, void *);
void *buddy_alloc(struct Buddy *, uint64);
void buddy_dealloc(struct Buddy *, void *, uint64);

/* elf.c */
struct MemoryMap *from_elf(char *);

/* fs.c */
void init_fs();
struct Inode *lookup(struct Inode *, char *);
int readall(struct Inode *, char *);

/* kerneltrap.S */
void __trap_entry();
void __restore(struct TrapContext *current_trap_cx);

/* memory.c */
void init_memory();
void *alloc(usize);
void dealloc(void *, usize);
usize alloc_frame();
void dealloc_frame(usize);

/* mapping.c */
struct Segment *new_segment(usize, usize, usize, enum SegmentType);
void map_segment(usize, struct Segment *, char *, usize);
void map_pages(usize, usize, usize, int, usize);
struct MemoryMap *new_kernel_mapping();
struct MemoryMap *remap_kernel();
void dealloc_memory_map(struct MemoryMap *);
struct MemoryMap *copy_mm(struct MemoryMap *);
void activate_pagetable(usize);
usize translate(usize, usize);

/* printf.c */
void printf(char *, ...);
void panic(char *, ...) __attribute__((noreturn));

/* sbi.c */
void console_putchar(usize);
usize console_getchar();
void shutdown() __attribute__((noreturn));
void set_timer(usize);

/* syscall.c */
usize syscall(usize, usize[3]);

/* switch.S */
void __switch(struct TaskContext *current_task_cx, struct TaskContext *next_task_cx);

/* trap.c */
void init_trap();

/* task.c */
void add_task(struct Task *);
void init_task();
void exit_current();
int sys_fork();
int sys_wait();
int sys_exec(char *);

/* timer.c */
void init_timer();
void set_next_timeout();

#endif