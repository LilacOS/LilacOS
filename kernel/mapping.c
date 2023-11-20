#include "types.h"
#include "def.h"
#include "consts.h"
#include "mapping.h"

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
 * 根据页表解析虚拟页号，若没有相应页表则会创建
 *
 * @param mapping 页表
 * @param vpn 虚拟页号
 * @return 解析得到的页表项
 */
PageTableEntry *find_entry(struct Mapping mapping, usize vpn)
{
    struct PageTable *root = (struct PageTable *)__va(mapping.root_ppn << 12);
    usize *levels = get_vpn_levels(vpn);
    PageTableEntry *pte = &(root->entries[levels[2]]);
    for (int i = 1; i >= 0; --i)
    {
        if (*pte == 0)
        {
            // 页表不存在，创建新页表
            usize new_ppn = alloc_frame();
            *pte = PPN2PTE(new_ppn, PAGE_VALID);
        }
        usize next_pa = PTE2PA(*pte);
        pte = &(((struct PageTable *)__va(next_pa))->entries[levels[i]]);
    }
    return pte;
}

/**
 * 新建页表
 */
struct Mapping new_mapping()
{
    struct Mapping res = {alloc_frame()};
    return res;
}

/**
 * 线性映射一个段，填充页表
 */
void map_linear_segment(struct Mapping mapping, struct Segment segment)
{
    usize start_vpn = segment.start_va >> 12;
    usize end_vpn = ((segment.end_va - 1) >> 12) + 1;
    for (usize vpn = start_vpn; vpn < end_vpn; ++vpn)
    {
        PageTableEntry *entry = find_entry(mapping, vpn);
        if (*entry != 0)
        {
            panic("Virtual address already mapped!\n");
        }
        *entry = PPN2PTE(__ppn(vpn), segment.flags);
    }
}

/**
 * 随机映射一个段，填充页表。
 *
 * @param mapping 页表
 * @param segment 需映射的段
 * @param data 如果非 NULL，则表示映射段的数据，需将该数据填充最终的物理页
 * @param len 数据的大小
 * @note 映射后的物理地址是随机的。
 */
void map_framed_segment(struct Mapping mapping, struct Segment segment, char *data, usize len)
{
    usize start_vpn = segment.start_va >> 12;
    usize end_vpn = ((segment.end_va - 1) >> 12) + 1;
    for (usize vpn = start_vpn; vpn < end_vpn; ++vpn)
    {
        PageTableEntry *entry = find_entry(mapping, vpn);
        if (*entry != 0)
        {
            panic("Virtual address already mapped!\n");
        }
        usize ppn = alloc_frame();
        *entry = PPN2PTE(ppn, segment.flags);
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
 * 激活页表
 */
void activate_mapping(struct Mapping mapping)
{
    usize satp = __satp(mapping.root_ppn);
    asm volatile("csrw satp, %0" : : "r"(satp));
    asm volatile("sfence.vma" :::);
}

/**
 * 新建内核映射，并返回页表（不激活）
 */
struct Mapping new_kernel_mapping()
{
    struct Mapping m = new_mapping();

    // .text 段，r-x
    struct Segment text = {
        (usize)stext,
        (usize)etext,
        PAGE_VALID | PAGE_READ | PAGE_EXEC};
    map_linear_segment(m, text);

    // .rodata 段，r--
    struct Segment rodata = {
        (usize)srodata,
        (usize)erodata,
        PAGE_VALID | PAGE_READ};
    map_linear_segment(m, rodata);

    // .data 段，rw-
    struct Segment data = {
        (usize)sdata,
        (usize)edata,
        PAGE_VALID | PAGE_READ | PAGE_WRITE};
    map_linear_segment(m, data);

    // .bss 段，rw-
    struct Segment bss = {
        (usize)sbss_with_stack,
        (usize)ebss,
        PAGE_VALID | PAGE_READ | PAGE_WRITE};
    map_linear_segment(m, bss);

    // 剩余空间，rw-
    struct Segment other = {
        (usize)ekernel,
        __va(MEMORY_END),
        PAGE_VALID | PAGE_READ | PAGE_WRITE};
    map_linear_segment(m, other);

    return m;
}

/**
 * 映射内核
 */
void map_kernel()
{
    struct Mapping m = new_kernel_mapping();
    activate_mapping(m);
    printf("***** Remap Kernel *****\n");
}