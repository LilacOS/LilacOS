#include "types.h"
#include "def.h"
#include "consts.h"
#include "task.h"
#include "riscv.h"
#include "elf.h"
#include "mapping.h"
#include "fs.h"

// 当前运行的进程
struct Task *current = NULL;
// 系统启动进程
struct Task *idle = NULL;
// 第一个创建的进程
struct Task *init = NULL;

#define MAX_PID 1024
static int pids[MAX_PID / 32] = {0};

/**
 * 分配 pid
 */
int alloc_pid() {
    for (int i = 0; i < MAX_PID / 32; ++i) {
        if (pids[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; ++j) {
                if ((pids[i] & (1 << j)) == 0) {
                    // 标记为已分配
                    pids[i] |= (1 << j);
                    return i * 32 + j;
                }
            }
        }
    }
    return -1;
}

/**
 * 释放 pid
 */
void dealloc_pid(int pid) {
    if (pid >= 0 && pid < MAX_PID) {
        int index = pid / 32;
        int offset = pid % 32;
        // 标记为未分配
        pids[index] &= ~(1 << offset);
    } else {
        panic("Invalid PID: %d\n", pid);
    }
}

/**
 * 初始化设置进程上下文使其返回 __restore
 *
 * @param task_cx 进程上下文
 * @param kernel_sp 内核栈栈顶
 */
void goto_trap_restore(struct TaskContext *task_cx, usize kernel_sp) {
    task_cx->ra = (usize)__restore;
    task_cx->sp = kernel_sp;
    for (int i = 0; i < 12; ++i) {
        task_cx->s[i] = 0;
    }
}

/**
 * 初始化 Trap 上下文使其返回用户入口函数
 *
 * @param trap_cx Trap 上下文
 * @param entry 用户入口函数
 * @param user_sp 用户栈栈顶
 * @param kernel_sp 内核栈栈顶
 */
void goto_app(struct TrapContext *trap_cx, usize entry, usize user_sp,
              usize kernel_sp) {
    for (int i = 0; i < 32; ++i) {
        trap_cx->x[i] = 0;
    }
    usize sstatus = r_sstatus();
    // 设置返回后的特权级为 U-Mode
    sstatus &= ~SSTATUS_SPP;
    // 中断使能
    sstatus |= SSTATUS_SPIE;
    sstatus &= ~SSTATUS_SIE;
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
struct Task *new_task(char *elf) {
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
    // 将用户栈映射到固定位置
    map_pages(mm->root_ppn, USER_STACK, __pa(res->ustack), USER_STACK_SIZE,
              PAGE_VALID | PAGE_USER | PAGE_READ | PAGE_WRITE);

    goto_trap_restore(&res->task_cx, res->kstack + KERNEL_STACK_SIZE);
    goto_app(&res->trap_cx, ((struct ElfHeader *)elf)->entry,
             USER_STACK + USER_STACK_SIZE, res->kstack + KERNEL_STACK_SIZE);

    INIT_LIST_HEAD(&res->list);
    INIT_LIST_HEAD(&res->children);
    INIT_LIST_HEAD(&res->sibling);
    return res;
}

/**
 * 添加进程到调度队列队尾中
 */
void add_task(struct Task *task) { list_add_tail(&task->list, &idle->list); }

/**
 * 进程调度
 */
void schedule() {
    while (!list_empty(&idle->list)) {
        struct Task *task;
        list_for_each_entry(task, &idle->list, list) {
            if (task->state == Ready) {
                list_del(&task->list);
                task->state = Running;
                idle->state = Ready;
                current = task;
                __switch(&idle->task_cx, &current->task_cx);
                current = idle;
                idle->state = Running;
                break;
            }
        }
    }
}

int sys_fork() {
    struct Task *child = (struct Task *)alloc(sizeof(struct Task));
    child->pid = alloc_pid();
    child->state = Ready;
    child->kstack = (usize)alloc(KERNEL_STACK_SIZE);
    child->ustack = (usize)alloc(USER_STACK_SIZE);
    child->parent = current;
    INIT_LIST_HEAD(&child->list);
    INIT_LIST_HEAD(&child->children);
    INIT_LIST_HEAD(&child->sibling);

    usize kernel_sp = child->kstack + KERNEL_STACK_SIZE;
    goto_trap_restore(&child->task_cx, kernel_sp);
    // 复制地址空间
    child->mm = copy_mm(current->mm);
    child->task_cx.satp = __satp(child->mm->root_ppn);
    // 将用户栈映射到固定位置
    map_pages(child->mm->root_ppn, USER_STACK, __pa(child->ustack),
              USER_STACK_SIZE, PAGE_VALID | PAGE_USER | PAGE_READ | PAGE_WRITE);
    // 复制用户栈（用户栈虚拟及物理地址均连续）
    char *src_stack = (char *)current->ustack;
    char *dst_stack = (char *)child->ustack;
    for (int i = 0; i < USER_STACK_SIZE; ++i) {
        dst_stack[i] = src_stack[i];
    }
    // 复制 Trap 上下文
    child->trap_cx = current->trap_cx;
    child->trap_cx.kernel_sp = kernel_sp;
    // 子进程返回 0
    child->trap_cx.x[10] = 0;

    list_add(&child->sibling, &current->children);
    add_task(child);
    return child->pid;
}

int sys_wait() {
    int flag = 0;
    int pid = -1;
    struct Task *child;
    while (1) {
        list_for_each_entry(child, &current->children, sibling) {
            if (child->state == Exited) { // 将子进程删除
                list_del(&child->sibling);
                // 回收进程的其余资源
                pid = child->pid;
                dealloc_pid(child->pid);
                dealloc((void *)child->kstack, KERNEL_STACK_SIZE);
                dealloc_memory_map(child->mm);
                dealloc((void *)child, sizeof(struct Task));
                return pid;
            } else {
                flag = 1;
            }
        }
        if (flag) { // 有子进程还没退出，挂起当前进程等待
            current->state = Ready;
            add_task(current);
            __switch(&current->task_cx, &idle->task_cx);
        } else { // 所有子进程均退出，返回 -1
            break;
        }
    }
    return -1;
}

int sys_exec(char *path) {
    struct Inode *inode = lookup(NULL, path);
    char *buf = (char *)alloc(inode->size);
    readall(inode, buf);
    struct MemoryMap *mm = from_elf(buf);

    // 激活新页表
    activate_pagetable(mm->root_ppn);
    // 替换进程地址空间
    struct MemoryMap *old_mm = current->mm;
    current->mm = mm;
    dealloc_memory_map(old_mm);
    current->task_cx.satp = __satp(mm->root_ppn);

    // 重新分配映射用户栈
    // 内核栈使用原来的内核栈即可
    dealloc((void *)current->ustack, USER_STACK_SIZE);
    current->ustack = (usize)alloc(USER_STACK_SIZE);
    // 将用户栈映射到固定位置
    map_pages(mm->root_ppn, USER_STACK, __pa(current->ustack), USER_STACK_SIZE,
              PAGE_VALID | PAGE_USER | PAGE_READ | PAGE_WRITE);

    goto_app(&current->trap_cx, ((struct ElfHeader *)buf)->entry,
             USER_STACK + USER_STACK_SIZE, current->kstack + KERNEL_STACK_SIZE);

    dealloc(buf, inode->size);
    return 0;
}

/**
 * 终止当前进程运行
 *
 * 只释放部分资源，剩余的资源将由 wait 系统调用回收
 */
void exit_current() {
    dealloc((void *)current->ustack, USER_STACK_SIZE);
    current->state = Exited;
    // 进程调度的时候已经将其从调度队列移除，不用再次移除
    // 将进程的所有子进程挂到 init 进程上
    if (current != init && current->parent != init) {
        while (!list_empty(&current->children)) {
            struct Task *child;
            list_for_each_entry(child, &current->children, sibling) {
                list_del(&child->sibling);
                list_add(&child->sibling, &init->children);
            }
        }
    }
    __switch(&current->task_cx, &idle->task_cx);
}

void init_task() {
    idle = (struct Task *)alloc(sizeof(struct Task));
    idle->pid = alloc_pid();
    idle->state = Running;
    INIT_LIST_HEAD(&idle->list);
    INIT_LIST_HEAD(&idle->children);
    INIT_LIST_HEAD(&idle->sibling);
    // 重新映射内核
    idle->mm = remap_kernel();

    // 从文件系统中读取 elf 文件
    struct Inode *init_inode = lookup(NULL, "init");
    char *buf = (char *)alloc(init_inode->size);
    readall(init_inode, buf);
    init = new_task(buf);
    add_task(init);
    dealloc(buf, init_inode->size);

    printf("***** Init Task *****\n");
    schedule();
}