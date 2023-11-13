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

#endif