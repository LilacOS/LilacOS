#ifndef _TASK_H
#define _TASK_H

#include "context.h"

#define MAX_TASKS 40

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
    // 进程与 Trap 上下文必须紧邻放置，在进程切换时
    // 会使用到它们之间的关系

    struct TaskContext task_cx;
    struct TrapContext trap_cx;
    usize kstack;
    usize ustack;
    enum TaskState state;
};

#endif