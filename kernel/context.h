#ifndef _CONTEXT_H
#define _CONTEXT_H

#include "types.h"

/**
 * Trap上下文
 */
struct TrapContext {
    // 32个通用寄存器
    usize x[32];
    // S-Mode 状态寄存器
    usize sstatus;
    // trap返回地址
    usize sepc;
    usize kernel_sp;
};

/**
 * 进程上下文
 */
struct TaskContext {
    usize ra;
    usize sp;
    usize s[12];
    usize satp;
};

#endif