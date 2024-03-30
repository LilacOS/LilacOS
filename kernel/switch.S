.altmacro
.macro SAVE_SN n
    sd s\n, (\n+2)*8(a0)
.endm
.macro LOAD_SN n
    ld s\n, (\n+2)*8(a1)
.endm
    .section .text
    .globl __switch
__switch:
    # __switch(
    #     ProcessContext *current_process_cx, 
    #     ProcessContext *next_process_cx
    # )
    # 保存当前进程的内核栈
    sd sp, 8(a0)
    # 保存当前进程的 ra、s0~s11、satp 寄存器
    sd ra, 0(a0)
    .set n, 0
    .rept 12
        SAVE_SN %n
        .set n, n + 1
    .endr
    csrr s0, satp
    sd s0, 14*8(a0)

    # 恢复目标进程的 ra、s0~s11 寄存器
    ld ra, 0(a1)
    .set n, 0
    .rept 12
        LOAD_SN %n
        .set n, n + 1
    .endr
    # 恢复目标进程的内核栈
    ld sp, 8(a1)
    # 将 a0 寄存器设置为 a1+sizeof(ProcessContext)，使其指向 Trap 上下文
    # 从而能使新建的进程在返回后跳转到 __restore() 继续执行
    addi a0, a1, 15*8
    # 恢复目标进程的 satp 寄存器，刷新 TLB 缓存
    ld s0, 14*8(a1)
    csrw satp, s0
    sfence.vma
    ret
