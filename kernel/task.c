#include "types.h"
#include "def.h"
#include "consts.h"
#include "task.h"
#include "riscv.h"
#include "elf.h"
#include "mapping.h"

// 当前运行的进程
struct Task *current = NULL;
struct Task *init = NULL;
struct TaskContext *idle = NULL;

int alloc_pid()
{
    return 0;
}

void dealloc_pid()
{
}

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
    struct Task *res = (struct Task *)alloc(sizeof(struct Task));
    res->pid = alloc_pid();
    res->state = Ready;

    // 设置根页表地址
    struct MemoryMap *mm = from_elf(elf);
    res->task_cx.satp = __satp(mm->root_ppn);
    res->mm = mm;
    // 分配内核栈，返回内核栈低地址
    res->kstack = (usize)alloc(KERNEL_STACK_SIZE);
    // 分配映射用户栈
    res->ustack = (usize)alloc(USER_STACK_SIZE);
    map_pages(mm->root_ppn, USER_STACK, __pa(res->ustack),
              USER_STACK_SIZE, PAGE_VALID | PAGE_USER | PAGE_READ | PAGE_WRITE);

    usize sstatus = r_sstatus();
    // 设置返回后的特权级为 U-Mode
    sstatus &= ~SSTATUS_SPP;
    // 中断使能
    sstatus |= SSTATUS_SPIE;
    sstatus &= ~SSTATUS_SIE;
    goto_trap_restore(&res->task_cx, res->kstack + KERNEL_STACK_SIZE);
    goto_app(&res->trap_cx, sstatus, ((struct ElfHeader *)elf)->entry,
             res->ustack + USER_STACK_SIZE, res->kstack + KERNEL_STACK_SIZE);

    INIT_LIST_HEAD(&res->list);
    return res;
}

/**
 * 添加进程到调度队列中
 */
void add_task(struct Task *task)
{
    list_add(&task->list, &init->list);
}

/**
 * 进程调度
 */
void schedule()
{
    while (1)
    {
        struct Task *task;
        list_for_each_entry(task, &init->list, list)
        {
            if (task->state == Ready)
            {
                list_del(&task->list);
                current = task;
                current->state = Running;
                __switch(&init->task_cx, &current->task_cx);
                break;
            }
        }
    }
}

/**
 * 终止当前进程运行
 * 释放进程资源，同时会将 current 设置为 NULL
 */
void exit_current()
{
    dealloc_pid();
    dealloc((void *)current->kstack, KERNEL_STACK_SIZE);
    dealloc((void *)current->ustack, USER_STACK_SIZE);
    dealloc((void *)current, sizeof(struct Task));
    // 当进程终止运行时，current 设置为 NULL
    current = NULL;
    __switch(idle, &init->task_cx);
}

void init_task()
{
    idle = (struct TaskContext *)alloc(sizeof(struct TaskContext));
    init = (struct Task *)alloc(sizeof(struct Task));
    init->pid = alloc_pid();
    init->state = Running;
    INIT_LIST_HEAD(&init->list);

    for (int i = 0; i < 5; ++i)
    {
        extern void _user_img_start();
        struct Task *task = new_task((char *)_user_img_start);
        add_task(task);
    }
    printf("***** Init Task *****\n");
    schedule();
}