#include "types.h"
#include "context.h"
#include "def.h"
#include "riscv.h"
#include "trap.h"
#include "task.h"

extern struct Task *current, *idle;

void init_trap() {
    // 设置 stvec 寄存器，设置中断处理函数和处理模式
    w_stvec((usize)__trap_entry | MODE_DIRECT);
    // 初始化时钟中断
    init_timer();
    printf("***** Init Trap *****\n");
}

void breakpoint(struct TrapContext *context) {
    printf("Breakpoint at %p\n", context->sepc);
    // ebreak 指令长度 2 字节，返回后跳过该条指令
    context->sepc += 2;
}

void syscall_handle(struct TrapContext *context) {
    context->sepc += 4;
    usize ret =
        syscall(context->x[17],
                (usize[]){context->x[10], context->x[11], context->x[12]});
    context->x[10] = ret;
}

void supervisor_timer(struct TrapContext *context) {
    set_next_timeout();
    current->state = Ready;
    add_task(current);
    __switch(&current->task_cx, &idle->task_cx);
}

void fault(struct TrapContext *context, usize scause, usize stval) {
    panic("Unhandled trap!\nscause\t= %p\nsepc\t= %p\nstval\t= %p\n", scause,
          context->sepc, stval);
}

void trap_handle(struct TrapContext *context, usize scause, usize stval) {
    switch (scause) {
    case BREAKPOINT:
        breakpoint(context);
        break;
    case USER_ENV_CALL:
        syscall_handle(context);
        break;
    case SUPERVISOR_TIMER:
        supervisor_timer(context);
        break;
    default:
        fault(context, scause, stval);
        break;
    }
    __restore(context);
}