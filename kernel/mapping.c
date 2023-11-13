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
PageTableEntry *find_entry(Mapping mapping, usize vpn)
{
    PageTable *root = (PageTable *)__va(mapping.root_ppn << 12);
    usize *levels = get_vpn_levels(vpn);
    PageTableEntry *pte = &(root->entries[levels[2]]);
    for (int i = 1; i >= 0; --i)
    {
        if (*pte == 0)
        {
            // 页表不存在，创建新页表
            usize new_ppn = alloc_frame();
            *pte = (new_ppn << 10) | VALID;
        }
        usize next_pa = pte_to_pa(*pte);
        pte = &(((PageTable *)__va(next_pa))->entries[levels[i]]);
    }
    return pte;
}

/**
 * 新建页表
 */
Mapping new_mapping()
{
    Mapping res = {alloc_frame()};
    return res;
}

/**
 * 线性映射一个段，填充页表
 */
void map_linear_segment(Mapping mapping, Segment segment)
{
    usize start_vpn = segment.start_va >> 12;
    usize end_vpn = segment.end_va >> 12;
    if ((segment.end_va & 0x3ff) == 0)
    {
        end_vpn--;
    }
    for (usize vpn = start_vpn; vpn <= end_vpn; ++vpn)
    {
        PageTableEntry *entry = find_entry(mapping, vpn);
        if (*entry != 0)
        {
            panic("Virtual address already mapped!\n");
        }
        *entry = ((vpn - KERNEL_PAGE_OFFSET) << 10) | segment.flags | VALID;
    }
}

/**
 * 激活页表
 */
void activate_mapping(Mapping mapping)
{
    usize satp = mapping.root_ppn | (8L << 60);
    asm volatile("csrw satp, %0" : : "r"(satp));
    asm volatile("sfence.vma" :::);
}

/**
 * 新建内核映射，并返回页表（不激活）
 */
Mapping new_kernel_mapping()
{
    Mapping m = new_mapping();

    // .text 段，r-x
    Segment text = {
        (usize)stext,
        (usize)etext,
        1L | READABLE | EXECUTABLE};
    map_linear_segment(m, text);

    // .rodata 段，r--
    Segment rodata = {
        (usize)srodata,
        (usize)erodata,
        1L | READABLE};
    map_linear_segment(m, rodata);

    // .data 段，rw-
    Segment data = {
        (usize)sdata,
        (usize)edata,
        1L | READABLE | WRITABLE};
    map_linear_segment(m, data);

    // .bss 段，rw-
    Segment bss = {
        (usize)sbss_with_stack,
        (usize)ebss,
        1L | READABLE | WRITABLE};
    map_linear_segment(m, bss);

    // 剩余空间，rw-
    Segment other = {
        (usize)ekernel,
        (usize)(MEMORY_END + KERNEL_MAP_OFFSET),
        1L | READABLE | WRITABLE};
    map_linear_segment(m, other);

    return m;
}

/**
 * 映射内核
 */
void map_kernel()
{
    Mapping m = new_kernel_mapping();
    activate_mapping(m);
    printf("***** Remap Kernel *****\n");
}