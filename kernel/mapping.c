#include "types.h"
#include "def.h"
#include "consts.h"
#include "mapping.h"

struct MemoryMap *new_mapping()
{
    struct MemoryMap *res = (struct MemoryMap *)alloc(sizeof(struct MemoryMap));
    res->root_ppn = alloc_frame();
    res->areas.next = &res->areas;
    return res;
}

struct Segment *new_segment(usize start_va, usize end_va, usize flags, enum SegmentType type)
{
    struct Segment *res = (struct Segment *)alloc(sizeof(struct Segment));
    res->start_va = start_va;
    res->end_va = end_va;
    res->flags = flags;
    res->type = type;
    res->list.next = &res->list;
    return res;
}

/**
 * 提取虚拟页号的各级页表号
 */
usize *get_vpn_levels(usize vpn)
{
    static usize res[3];
    res[2] = (vpn >> 18) & 0x1ff;
    res[1] = (vpn >> 9) & 0x1ff;
    res[0] = vpn & 0x1ff;
    return res;
}

/**
 * 根据页表解析虚拟页号
 *
 * @param root_ppn 根页表物理地址
 * @param vpn 虚拟页号
 * @param flag 表示页表不存在时是否需要创建
 * @return 解析得到的页表项指针。若找不到则返回 NULL
 */
PageTableEntry *find_entry(usize root_ppn, usize vpn, int flag)
{
    PageTable root = (PageTable)__va(root_ppn << 12);
    usize *levels = get_vpn_levels(vpn);
    PageTableEntry *pte = &(root[levels[2]]);
    for (int i = 1; i >= 0; --i)
    {
        if (*pte == 0)
        {
            if (flag)
            { // 页表不存在，创建新页表
                usize new_ppn = alloc_frame();
                *pte = PPN2PTE(new_ppn, PAGE_VALID);
            }
            else
            {
                return NULL;
            }
        }
        usize next_pa = PTE2PA(*pte);
        pte = &(((PageTable)__va(next_pa))[levels[i]]);
    }
    return pte;
}

/**
 * 映射一个段，填充页表。
 *
 * @param root_ppn 根页表物理地址
 * @param segment 需映射的段
 * @param data 如果非 NULL，则表示映射段的数据，需将该数据填充最终的物理页
 * @param len 数据的大小
 */
void map_segment(usize root_ppn, struct Segment *segment, char *data, usize len)
{
    usize start_vpn = segment->start_va >> 12;
    usize end_vpn = ((segment->end_va - 1) >> 12) + 1;
    for (usize vpn = start_vpn; vpn < end_vpn; ++vpn)
    {
        PageTableEntry *entry = find_entry(root_ppn, vpn, 1);
        if (*entry != 0)
        {
            panic("[map_segment] Virtual address already mapped!\n");
        }
        usize ppn = segment->type == Linear ? __ppn(vpn) : alloc_frame();
        *entry = PPN2PTE(ppn, segment->flags);
        if (data)
        { // 复制数据到目标位置
            char *dst = (char *)__va(ppn << 12);
            usize size = len >= PAGE_SIZE ? PAGE_SIZE : len;
            for (int i = 0; i < size; ++i)
            {
                dst[i] = data[i];
            }
            data += size;
            len -= size;
        }
    }
}

/**
 * 释放映射段数据页（非页表本身）
 *
 * @param root_ppn 根页表物理地址
 * @param segment 需释放的段
 */
void unmap_segment(usize root_ppn, struct Segment *segment)
{
    if (segment->type == Linear)
    {
        return;
    }
    usize start_vpn = segment->start_va >> 12;
    usize end_vpn = ((segment->end_va - 1) >> 12) + 1;
    for (usize vpn = start_vpn; vpn < end_vpn; ++vpn)
    {
        PageTableEntry *entry = find_entry(root_ppn, vpn, 0);
        if (!entry)
        {
            panic("[unmap_segment] Can't find pte\n");
        }
        usize ppn = PTE2PPN(*entry);
        dealloc_frame(ppn);
    }
}

/**
 * 释放页表内存
 *
 * @param ppn 页表所在物理地址
 */
void dealloc_pagetable(usize ppn)
{
    PageTable pagetable = (PageTable)__va(ppn << 12);
    for (int i = 0; i < 512; ++i)
    {
        PageTableEntry pte = pagetable[i];
        if ((pte & PAGE_VALID) && (pte & (PAGE_READ | PAGE_WRITE | PAGE_EXEC)) == 0)
        { // 下级页表
            usize next_ppn = PTE2PPN(pte);
            dealloc_pagetable(next_ppn);
            pagetable[i] = 0;
        }
    }
    dealloc_frame(ppn);
}

/**
 * 激活页表
 */
void activate_pagetable(usize root_ppn)
{
    usize satp = __satp(root_ppn);
    asm volatile("csrw satp, %0" : : "r"(satp));
    asm volatile("sfence.vma" :::);
}

/**
 * 新建内核映射，并返回页表（不激活）
 */
struct MemoryMap *new_kernel_mapping()
{
    struct MemoryMap *mm = new_mapping();

    // .text 段，r-x
    struct Segment *text = new_segment(
        (usize)stext, (usize)etext,
        PAGE_VALID | PAGE_READ | PAGE_EXEC,
        Linear);
    map_segment(mm->root_ppn, text, NULL, 0);

    // .rodata 段，r--
    struct Segment *rodata = new_segment(
        (usize)srodata, (usize)erodata,
        PAGE_VALID | PAGE_READ,
        Linear);
    map_segment(mm->root_ppn, rodata, NULL, 0);

    // .data 段，rw-
    struct Segment *data = new_segment(
        (usize)sdata, (usize)edata,
        PAGE_VALID | PAGE_READ | PAGE_WRITE,
        Linear);
    map_segment(mm->root_ppn, data, NULL, 0);

    // .bss 段，rw-
    struct Segment *bss = new_segment(
        (usize)sbss_with_stack, (usize)ebss,
        PAGE_VALID | PAGE_READ | PAGE_WRITE,
        Linear);
    map_segment(mm->root_ppn, bss, NULL, 0);

    // 剩余空间，rw-
    struct Segment *other = new_segment(
        (usize)ekernel, __va(MEMORY_END),
        PAGE_VALID | PAGE_READ | PAGE_WRITE,
        Linear);
    map_segment(mm->root_ppn, other, NULL, 0);

    // 连接各个映射区域
    list_add(&other->list, &mm->areas);
    list_add(&bss->list, &mm->areas);
    list_add(&data->list, &mm->areas);
    list_add(&rodata->list, &mm->areas);
    list_add(&text->list, &mm->areas);

    return mm;
}

/**
 * 映射内核
 */
void map_kernel()
{
    struct MemoryMap *mm = new_kernel_mapping();
    activate_pagetable(mm->root_ppn);
    printf("***** Remap Kernel *****\n");
}