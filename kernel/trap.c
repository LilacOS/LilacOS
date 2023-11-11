#include "types.h"
#include "context.h"
#include "def.h"
#include "riscv.h"
#include "trap.h"

void init_trap()
{
    // 设置 stvec 寄存器，设置中断处理函数和处理模式
    extern void __trap_entry();
    w_stvec((usize)__trap_entry | MODE_DIRECT);
    // 初始化时钟中断
    extern void init_timer();
    init_timer();
    // 监管者模式中断使能
    w_sstatus(r_sstatus() | SSTATUS_SIE);
    printf("***** Init Trap *****\n");
}

void breakpoint(Context *context)
{
    printf("Breakpoint at %p\n", context->sepc);
    // ebreak 指令长度 2 字节，返回后跳过该条指令
    context->sepc += 2;
}

void supervisor_timer(Context *context)
{
    extern void set_next_timeout();
    set_next_timeout();
}

void fault(Context *context, usize scause, usize stval)
{
    printf("Unhandled trap!\nscause\t= %p\nsepc\t= %p\nstval\t= %p\n",
           scause,
           context->sepc,
           stval);
    panic("");
}

void trap_handle(Context *context, usize scause, usize stval)
{
    switch (scause)
    {
    case BREAKPOINT:
        breakpoint(context);
        break;
    case SUPERVISOR_TIMER:
        supervisor_timer(context);
        break;
    default:
        fault(context, scause, stval);
        break;
    }
}