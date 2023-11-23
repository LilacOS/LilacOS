#include "types.h"
#include "def.h"
#include "consts.h"
#include "task.h"
#include "riscv.h"
#include "elf.h"
#include "mapping.h"

// 进程调度队列
struct Task *tasks[MAX_TASKS] = {NULL};
// 当前运行的进程
struct Task *current = NULL;
// 退出的进程由于资源已经释放，因此使用 idle 作为一个空的
// 进程上下文用于进程切换
struct TaskContext *idle = NULL;

/**
 * 初始化设置进程上下文使其返回 __restore
 *
 * @param task_cx 进程上下文
 * @param kernel_sp 内核栈栈顶
 */
void goto_trap_restore(struct TaskContext *task_cx, usize kernel_sp)
{
    task_cx->ra = (usize)__restore;
    task_cx->sp = kernel_sp;
    for (int i = 0; i < 12; ++i)
    {
        task_cx->s[i] = 0;
    }
}

/**
 * 初始化 Trap 上下文使其返回用户入口函数
 *
 * @param trap_cx Trap 上下文
 * @param sstatus sstatus 寄存器
 * @param entry 用户入口函数
 * @param user_sp 用户栈栈顶
 * @param kernel_sp 内核栈栈顶
 */
void goto_app(struct TrapContext *trap_cx, usize sstatus, usize entry,
              usize user_sp, usize kernel_sp)
{
    for (int i = 0; i < 32; ++i)
    {
        trap_cx->x[i] = 0;
    }
    trap_cx->sstatus = sstatus;
    trap_cx->sepc = entry;
    // 设置用户栈
    trap_cx->x[2] = user_sp;
    // 设置内核栈
    trap_cx->kernel_sp = kernel_sp;
}

/**
 * 创建新进程
 */
struct Task *new_task(char *elf)
{
    struct MemoryMap *mapping = new_user_mapping(elf);

    // 设置根页表地址
    struct Task *res = (struct Task *)alloc(sizeof(struct Task));
    res->task_cx.satp = __satp(mapping->root_ppn);

    // 分配内核栈，返回内核栈低地址
    res->kstack = (usize)alloc(KERNEL_STACK_SIZE);
    // 分配映射用户栈
    res->ustack = USER_STACK_OFFSET;
    usize ustack_bottom = USER_STACK_OFFSET, ustack_top = USER_STACK_OFFSET + USER_STACK_SIZE;
    struct Segment *stack = new_segment(ustack_bottom, ustack_top, PAGE_VALID | PAGE_USER | PAGE_READ | PAGE_WRITE, Framed);
    map_segment(mapping->root_ppn, stack, NULL, 0);

    usize sstatus = r_sstatus();
    // 设置返回后的特权级为 U-Mode
    sstatus &= ~SSTATUS_SPP;
    // 异步中断使能
    sstatus |= SSTATUS_SPIE;
    sstatus &= ~SSTATUS_SIE;

    goto_trap_restore(&res->task_cx, res->kstack + KERNEL_STACK_SIZE);
    goto_app(&res->trap_cx, sstatus, ((struct ElfHeader *)elf)->entry,
             res->ustack + USER_STACK_SIZE, res->kstack + KERNEL_STACK_SIZE);
    return res;
}

/**
 * 添加进程到调度队列中
 */
void add_task(struct Task *task)
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

/**
 * 从调度队列中寻找下一个准备好的进程返回
 */
struct Task *fetch_task()
{
    // 下一个首先查找的进程下标
    static int idx = 0;
    struct Task *res = NULL;
    for (int i = 0; i < MAX_TASKS; ++i)
    {
        int p = (idx + i) % MAX_TASKS;
        if (tasks[p])
        {
            if (tasks[p]->state == Ready)
            {
                res = tasks[p];
                idx = (p + 1) % MAX_TASKS;
                break;
            }
            else if (!current)
            { // 如果进程已经结束，则将其从调度队列中删除
                tasks[p] = NULL;
            }
        }
    }
    return res;
}

/**
 * 终止当前进程运行
 * 释放进程资源，同时会将 current 设置为 NULL
 */
void exit_current()
{
    dealloc((void *)current->kstack, KERNEL_STACK_SIZE);
    dealloc((void *)current, sizeof(struct Task));
    // 当进程终止运行时，current 设置为 NULL
    current = NULL;
    schedule();
}

/**
 * 进程调度
 */
void schedule()
{
    while (1)
    {
        struct Task *next = fetch_task();
        if (!next)
        { // 没有下一个准备好的进程且当前有进程准备切换
            // 则让该进程再运行一个时钟周期
            if (current)
            {
                break;
            }
        }
        else
        { // 找到进程，进行进程切换
            // 如果当前进程已经退出，则使用 idle 作为临时进程上下文辅助切换
            struct TaskContext *prev = current ? (current->state = Ready, &current->task_cx) : idle;
            current = next;
            next->state = Running;
            __switch(prev, &current->task_cx);
        }
    }
}

void init_task()
{
    idle = (struct TaskContext *)alloc(sizeof(struct TaskContext));
    for (int i = 0; i < 5; ++i)
    {
        extern void _user_img_start();
        struct Task *task = new_task((char *)_user_img_start);
        add_task(task);
    }
    printf("***** Init Task *****\n");
    schedule();
}