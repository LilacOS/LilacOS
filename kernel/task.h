#ifndef _TASK_H
#define _TASK_H

#include "context.h"

/**
 * 进程控制块
 */
typedef struct
{
    TaskContext task_cx;
    TrapContext trap_cx;
    usize kstack;
} Task;

#endif