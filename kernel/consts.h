#ifndef _CONSTS_H
#define _CONSTS_H

// 链接脚本的相关符号
extern void stext();
extern void etext();
extern void srodata();
extern void erodata();
extern void sdata();
extern void edata();
extern void sbss_with_stack();
extern void sbss();
extern void ebss();
extern void ekernel();

#define PAGE_SIZE 4096
#define MEMORY_END 0x88000000

// 内核地址线性映射偏移
#define KERNEL_MAP_OFFSET 0xffffffff00000000
// 内核页面线性映射偏移
#define KERNEL_PAGE_OFFSET 0xffffffff00000

#endif