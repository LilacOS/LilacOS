.altmacro
# 宏：保存寄存器到栈上
.macro SAVE_N n
    sd x\n, \n*8(sp)
.endm

# 宏：从栈中恢复寄存器
.macro LOAD_N n
    ld x\n, \n*8(sp)
.endm


    .section .text
    .globl __trap_entry
    # 中断处理函数需要 4 字节对齐
    .balign 4
# 全局中断处理，保存 Trap 上下文并跳转到 trap_handle() 处
# sscratch 保存 Trap 上下文的地址
__trap_entry:
    csrrw sp, sscratch, sp
    # 此时 sscratch 指向用户栈，sp 指向 Trap 上下文
    # 保存通用寄存器，其中 x0 固定为 0
    sd x1, 1*8(sp)
    # 循环保存 x3 至 x31
    .set n, 3
    .rept   29
        SAVE_N %n
        .set n, n + 1
    .endr
    # 保存 sp
    csrr s0, sscratch
    sd s0, 2*8(sp)

    # 保存 CSR
    csrr s1, sstatus
    csrr s2, sepc
    sd s1, 32*8(sp)
    sd s2, 33*8(sp)

    # 调用 trap_handle()
    # 将 TrapContext 的地址(栈顶)和 scause、stval 作为参数传入
    mv a0, sp
    csrr a1, scause
    csrr a2, stval
    # 加载内核栈
    ld sp, 34*8(sp)
    j trap_handle


    .globl __restore
# 从 Trap 上下文中恢复所有寄存器，并跳转至 Trap 上下文中 sepc 的位置
__restore:
    # __restore(
    #     TrapContext *current_trap_cx
    # )
    # 将 Trap 上下文地址保存到 sscratch，并加载到 sp
    csrw sscratch, a0
    mv sp, a0
    # 恢复 CSR
    ld s1, 32*8(sp)
    ld s2, 33*8(sp)
    csrw sstatus, s1
    csrw sepc, s2

    # 恢复通用寄存器
    ld x1, 1*8(sp)
    # 恢复 x3 至 x31
    .set n, 3
    .rept 29
        LOAD_N %n
        .set n, n + 1
    .endr

    # 恢复 sp（这里最后恢复是为了上面可以正常使用 LOAD 宏）
    ld sp, 2*8(sp)
    sret
