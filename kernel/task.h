#ifndef _TASK_H
#define _TASK_H

#include "def.h"
#include "list.h"
#include "context.h"

enum TaskState
{
    Ready,
    Running,
};

/**
 * 进程控制块
 */
struct Task
{
    int pid;
    enum TaskState state;
    // 进程与 Trap 上下文必须紧邻放置，在进程切换时
    // 会使用到它们之间的关系
    struct TaskContext task_cx;
    struct TrapContext trap_cx;
    usize kstack;
    usize ustack;
    struct MemoryMap *mm;
    struct list_head list;
};

#endif