#include "types.h"
#include "def.h"
#include "consts.h"
#include "task.h"
#include "riscv.h"

// 进程调度队列
Task *tasks[MAX_TASKS] = {NULL};
// 当前运行的进程
Task *current = NULL;
// 退出的进程由于资源已经释放，因此使用 idle 作为一个空的
// 进程上下文用于进程切换
TaskContext *idle = NULL;

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

void goto_app(TrapContext *trap_cx, usize sstatus, usize entry,
              usize app_sp, usize kernel_sp)
{
    for (int i = 0; i < 32; ++i)
    {
        trap_cx->x[i] = 0;
    }
    trap_cx->sstatus = sstatus;
    trap_cx->sepc = entry;
    // 设置用户栈
    trap_cx->x[2] = app_sp;
    // 设置内核栈
    trap_cx->kernel_sp = kernel_sp;
}

Task *new_task(usize entry, usize sstatus)
{
    Task *res = (Task *)alloc(sizeof(Task));
    // 分配内核栈，返回内核栈低地址
    res->kstack = (usize)alloc(KERNEL_STACK_SIZE);
    // 分配用户栈
    res->ustack = (usize)alloc(USER_STACK_SIZE);
    goto_trap_restore(&res->task_cx, res->kstack + KERNEL_STACK_SIZE);
    goto_app(&res->trap_cx, sstatus, entry,
             res->ustack + USER_STACK_SIZE, res->kstack + KERNEL_STACK_SIZE);
    return res;
}

void schedule()
{
    // 当前进程的下标
    static int idx = 0;
    Task *prev = NULL;
    for (int i = 1; i <= MAX_TASKS; ++i) {
        int next = (idx + i) % MAX_TASKS;
        if (tasks[next] && tasks[next]->state == Ready) {
            tasks[next]->state = Running;
            current->state = Ready;
            prev = current;
            current = tasks[next];
            idx = next;
            break;
        }
    }
    // 如果找到准备好的进程，切换
    if (prev) {
        __switch(&prev->task_cx, &current->task_cx);
    }
}

void add_task(Task *task)
{
    for (int i = 0; i < MAX_TASKS; ++i)
    {
        if (!tasks[i])
        {
            tasks[i] = task;
            break;
        }
    }
}

void init_task()
{
    printf("***** Init Task *****\n");
    idle = (TaskContext *)alloc(sizeof(TaskContext));
    todo();
}