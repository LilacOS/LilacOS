#ifndef _MAPPING_H
#define _MAPPING_H

#include "types.h"
#include "consts.h"

#define __va(pa) ((pa) + KERNEL_MAP_OFFSET)
#define __pa(va) ((va)-KERNEL_MAP_OFFSET)
#define __vpn(ppn) ((ppn) + KERNEL_PAGE_OFFSET)
#define __ppn(vpn) ((vpn)-KERNEL_PAGE_OFFSET)
#define __satp(ppn) ((ppn) | (8L << 60))
#define PTE2PA(pte) ((((usize)pte) & 0x003ffffffffffC00) << 2)
#define PPN2PTE(ppn, flags) (((ppn) << 10) | (flags))

typedef usize PageTableEntry;

struct PageTable
{
    PageTableEntry entries[PAGE_SIZE >> 3];
};

// 页表项的 8 个标志位
#define PAGE_VALID (1 << 0)
#define PAGE_READ (1 << 1)
#define PAGE_WRITE (1 << 2)
#define PAGE_EXEC (1 << 3)
#define PAGE_USER (1 << 4)
#define PAGE_GLOBAL (1 << 5)
#define PAGE_ACCESS (1 << 6)
#define PAGE_DIRTY (1 << 7)

/**
 * 映射片段，描述一个映射的行为
 */
struct Segment
{
    // 映射虚拟地址范围
    usize start_va;
    usize end_va;
    // 映射的权限标志
    usize flags;
};

/**
 * 页表，某个进程的内存映射关系
 */
struct Mapping
{
    // 根页表的物理页号
    usize root_ppn;
};

#endif