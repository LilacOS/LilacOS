#ifndef _TASK_H
#define _TASK_H

#include "context.h"

#define MAX_TASKS 40

typedef enum
{
    Ready,
    Running,
} TaskState;

/**
 * 进程控制块
 */
typedef struct
{
    // 进程与 Trap 上下文必须紧邻放置，在进程切换时
    // 会使用到它们之间的关系

    TaskContext task_cx;
    TrapContext trap_cx;
    usize kstack;
    usize ustack;
    TaskState state;
} Task;

#endif