#ifndef _TASK_H
#define _TASK_H

#include "context.h"

typedef struct {
    TaskContext task_cx;
    TrapContext trap_cx;
    // 线程栈底地址
    usize kstack;
} Task;

#endif