#include "types.h"
#include "def.h"
#include "consts.h"
#include "task.h"
#include "riscv.h"

extern void __switch(TaskContext *current_task_cx, TaskContext *next_task_cx);

void goto_trap_restore(TaskContext *task_cx, usize kstack_ptr)
{
    extern void __restore();
    task_cx->ra = (usize)__restore;
    task_cx->sp = kstack_ptr;
    for (int i = 0; i < 12; ++i)
    {
        task_cx->s[i] = 0;
    }
    task_cx->satp = r_satp();
}

void goto_app(TrapContext *trap_cx, usize sstatus, usize entry, usize app_sp, usize kernel_sp)
{
    for (int i = 0; i < 32; ++i)
    {
        trap_cx->x[i] = 0;
    }
    trap_cx->sstatus = sstatus;
    trap_cx->sepc = entry;
    // 设置应用栈
    trap_cx->x[2] = app_sp;
    // 设置内核栈
    trap_cx->kernel_sp = kernel_sp;
}

Task *new_task(usize entry)
{
    Task *res = (Task *)alloc(sizeof(Task));
    // 分配内核栈，返回内核栈低地址
    res->kstack = (usize)alloc(KERNEL_STACK_SIZE);
    // 分配应用栈
    usize ustack = (usize)alloc(APP_STACK_SIZE);
    // 设置 SPP 为 S 态
    usize sstatus = r_sstatus();
    sstatus |= SSTATUS_SPP;
    goto_trap_restore(&res->task_cx, res->kstack + KERNEL_STACK_SIZE);
    goto_app(&res->trap_cx, sstatus, entry, ustack + APP_STACK_SIZE, res->kstack + KERNEL_STACK_SIZE);
    return res;
}

void func(TaskContext *current, TaskContext *next, usize c)
{
    printf("The char passed by is ");
    console_putchar(c);
    console_putchar('\n');
    printf("Hello world from tempThread!\n");
    __switch(current, next);
}

void init_task()
{
    TaskContext *idle = (TaskContext *)alloc(sizeof(TaskContext));
    Task *tmp = new_task((usize)func);
    tmp->trap_cx.x[10] = (usize)&tmp->task_cx;
    tmp->trap_cx.x[11] = (usize)idle;
    tmp->trap_cx.x[12] = (usize)'M';
    __switch(idle, &tmp->task_cx);
    printf("I'm back from tempThread!\n");
}