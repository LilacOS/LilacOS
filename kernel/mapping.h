#ifndef _MAPPING_H
#define _MAPPING_H

#include "types.h"
#include "consts.h"

#define __va(pa) ((pa) + KERNEL_MAP_OFFSET)
#define __pa(va) ((va) - KERNEL_MAP_OFFSET)
#define pte_to_pa(pte) (((pte) & 0x003ffffffffffC00) << 2)

typedef usize PageTableEntry;

typedef struct
{
    PageTableEntry entries[PAGE_SIZE >> 3];
} PageTable;

// 页表项的 8 个标志位
#define VALID       (1 << 0)
#define READABLE    (1 << 1)
#define WRITABLE    (1 << 2)
#define EXECUTABLE  (1 << 3)
#define USER        (1 << 4)
#define GLOBAL      (1 << 5)
#define ACCESSED    (1 << 6)
#define DIRTY       (1 << 7)

/**
 * 映射片段，描述一个映射的行为
 */
typedef struct
{
    // 映射虚拟地址范围
    usize start_va;
    usize end_va;
    // 映射的权限标志
    usize flags;
} Segment;

/**
 * 页表，某个进程的内存映射关系
 */
typedef struct
{
    // 根页表的物理页号
    usize root_ppn;
} Mapping;

#endif