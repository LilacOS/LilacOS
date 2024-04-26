#include "types.h"
#include "def.h"
#include "consts.h"
#include "process.h"
#include "riscv.h"
#include "elf.h"
#include "mapping.h"
#include "fs.h"

// 当前运行的进程
struct ProcessControlBlock *current = NULL;
// 系统启动进程
struct ProcessControlBlock *idle = NULL;
// 第一个创建的进程
struct ProcessControlBlock *init = NULL;

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
 * @param process_cx 进程上下文
 * @param kernel_sp 内核栈栈顶
 */
void goto_trap_restore(struct ProcessContext *process_cx, usize kernel_sp) {
    process_cx->ra = (usize)__restore;
    process_cx->sp = kernel_sp;
    for (int i = 0; i < 12; ++i) {
        process_cx->s[i] = 0;
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
struct ProcessControlBlock *new_process(char *elf) {
    struct ProcessControlBlock *res =
        (struct ProcessControlBlock *)alloc(sizeof(struct ProcessControlBlock));
    res->pid = alloc_pid();
    res->state = Ready;

    // 设置根页表地址
    struct MemoryMap *mm = from_elf(elf);
    res->process_cx.satp = __satp(mm->root_ppn);
    res->mm = mm;
    // 分配内核栈，返回内核栈低地址
    res->kstack = (usize)alloc(KERNEL_STACK_SIZE);
    // 分配映射用户栈
    res->ustack = (usize)alloc(USER_STACK_SIZE);
    // 将用户栈映射到固定位置
    map_pages(mm->root_ppn, USER_STACK, __pa(res->ustack), USER_STACK_SIZE,
              PAGE_VALID | PAGE_USER | PAGE_READ | PAGE_WRITE);

    goto_trap_restore(&res->process_cx, res->kstack + KERNEL_STACK_SIZE);
    goto_app(&res->trap_cx, ((struct ElfHeader *)elf)->e_entry,
             USER_STACK + USER_STACK_SIZE, res->kstack + KERNEL_STACK_SIZE);

    INIT_LIST_HEAD(&res->list);
    INIT_LIST_HEAD(&res->children);
    INIT_LIST_HEAD(&res->sibling);

    for (int i = 0; i < NR_OPEN; ++i) {
        res->files[i] = NULL;
    }
    return res;
}

/**
 * 添加进程到调度队列队尾中
 */
void add_process(struct ProcessControlBlock *process) {
    list_add_tail(&process->list, &idle->list);
}

/**
 * 进程调度
 */
void schedule() {
    while (!list_empty(&idle->list)) {
        struct ProcessControlBlock *process;
        list_for_each_entry(process, &idle->list, list) {
            if (process->state == Ready) {
                list_del(&process->list);
                process->state = Running;
                idle->state = Ready;
                current = process;
                __switch(&idle->process_cx, &current->process_cx);
                current = idle;
                idle->state = Running;
                break;
            }
        }
    }
}

int sys_fork() {
    struct ProcessControlBlock *child =
        (struct ProcessControlBlock *)alloc(sizeof(struct ProcessControlBlock));
    child->pid = alloc_pid();
    child->state = Ready;
    child->kstack = (usize)alloc(KERNEL_STACK_SIZE);
    child->ustack = (usize)alloc(USER_STACK_SIZE);
    child->parent = current;
    INIT_LIST_HEAD(&child->list);
    INIT_LIST_HEAD(&child->children);
    INIT_LIST_HEAD(&child->sibling);

    usize kernel_sp = child->kstack + KERNEL_STACK_SIZE;
    goto_trap_restore(&child->process_cx, kernel_sp);
    // 复制地址空间
    child->mm = copy_mm(current->mm);
    child->process_cx.satp = __satp(child->mm->root_ppn);
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

    // 复制打开文件表
    for (int i = 0; i < NR_OPEN; ++i) {
        child->files[i] = current->files[i];
        if (child->files[i]) {
            ++(child->files[i]->count);
        }
    }

    list_add(&child->sibling, &current->children);
    add_process(child);
    return child->pid;
}

int sys_wait() {
    int flag = 0;
    int pid = -1;
    struct ProcessControlBlock *child;
    while (1) {
        list_for_each_entry(child, &current->children, sibling) {
            if (child->state == Exited) { // 将子进程删除
                list_del(&child->sibling);
                // 回收进程资源
                pid = child->pid;
                dealloc_pid(child->pid);
                dealloc_files(child->files);
                dealloc((void *)child->ustack, USER_STACK_SIZE);
                dealloc((void *)child->kstack, KERNEL_STACK_SIZE);
                dealloc_memory_map(child->mm);
                dealloc((void *)child, sizeof(struct ProcessControlBlock));
                return pid;
            } else {
                flag = 1;
            }
        }
        if (flag) { // 有子进程还没退出，挂起当前进程等待
            current->state = Ready;
            add_process(current);
            __switch(&current->process_cx, &idle->process_cx);
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
    current->process_cx.satp = __satp(mm->root_ppn);

    // 重新分配映射用户栈
    // 内核栈使用原来的内核栈即可
    dealloc((void *)current->ustack, USER_STACK_SIZE);
    current->ustack = (usize)alloc(USER_STACK_SIZE);
    // 将用户栈映射到固定位置
    map_pages(mm->root_ppn, USER_STACK, __pa(current->ustack), USER_STACK_SIZE,
              PAGE_VALID | PAGE_USER | PAGE_READ | PAGE_WRITE);

    // 打开文件表取消共享
    dealloc_files(current->files);

    goto_app(&current->trap_cx, ((struct ElfHeader *)buf)->e_entry,
             USER_STACK + USER_STACK_SIZE, current->kstack + KERNEL_STACK_SIZE);

    dealloc(buf, inode->size);
    return 0;
}

/**
 * 终止当前进程运行
 */
void exit_current() {
    current->state = Exited;
    // 进程调度的时候已经将其从调度队列移除，不用再次移除
    // 将进程的所有子进程挂到 init 进程上
    if (current != init) {
        while (!list_empty(&current->children)) {
            struct ProcessControlBlock *child;
            list_for_each_entry(child, &current->children, sibling) {
                list_del(&child->sibling);
                list_add(&child->sibling, &init->children);
            }
        }
    }
    __switch(&current->process_cx, &idle->process_cx);
}

void init_process() {
    idle =
        (struct ProcessControlBlock *)alloc(sizeof(struct ProcessControlBlock));
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
    init = new_process(buf);
    add_process(init);
    dealloc(buf, init_inode->size);

    printf("***** Init Task *****\n");
    schedule();
}