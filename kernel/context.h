#ifndef _CONTEXT_H
#define _CONTEXT_H

#include "types.h"

/**
 * Trap上下文
 */
typedef struct
{
    // 32个通用寄存器
    usize x[32];
    // S-Mode 状态寄存器
    usize sstatus;
    // trap返回地址
    usize sepc;
} TrapContext;

#endif