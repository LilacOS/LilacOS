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
    # 异常处理函数需要 4 字节对齐
    .balign 4
# 异常处理入口，保存 Trap 上下文并跳转到 trap_handle() 处
__trap_entry:
    # 移动栈指针，留出 Trap 上下文的空间
    addi sp, sp, -34*8
    
    # 保存通用寄存器，其中 x0 固定为 0
    sd x1, 1*8(sp)
    # 将原来的 sp 写入 2 位置
    addi x1, sp, 34*8
    sd x1, 2*8(sp)
    # 循环保存 x3 至 x31
    .set n, 3
    .rept 29
        SAVE_N %n
        .set n, n + 1
    .endr

    # 保存 CSR
    csrr s1, sstatus
    csrr s2, sepc
    sd s1, 32*8(sp)
    sd s2, 33*8(sp)

    # 调用 trap_handle()
    # 将 Trap 上下文的地址(栈顶)和 scause、stval 作为参数传入
    mv a0, sp
    csrr a1, scause
    csrr a2, stval
    call trap_handle


    .globl __restore
# 从 trap_handle 返回
# 从 Trap 上下文中恢复所有寄存器，并跳转至 Trap 上下文中 sepc 的位置
__restore:
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
    ld x2, 2*8(sp)
    sret