#include "types.h"
#include "def.h"
#include "mapping.h"
#include "elf.h"

/**
 * 将 ELF 标志位转换为页表项标志位
 */
usize convert_flags(uint32 flags) {
    usize res = PAGE_VALID | PAGE_USER;
    if (flags & ELF_PROG_FLAG_EXEC) {
        res |= PAGE_EXEC;
    }
    if (flags & ELF_PROG_FLAG_WRITE) {
        res |= PAGE_WRITE;
    }
    if (flags & ELF_PROG_FLAG_READ) {
        res |= PAGE_READ;
    }
    return res;
}

/**
 * 根据 ELF 文件新建进程地址空间（包含内核及用户）
 *
 * @param elf 用户可执行文件数据指针
 * @return 进程地址空间
 * @note 不包含用户栈及内核栈
 */
struct MemoryMap *from_elf(char *elf) {
    struct MemoryMap *res = new_kernel_memory_map();
    struct ElfHeader *e_header = (struct ElfHeader *)elf;
    // 校验 ELF 头
    if (!(e_header->e_ident[0] == 0x7f && e_header->e_ident[1] == 'E' &&
          e_header->e_ident[2] == 'L' && e_header->e_ident[3] == 'F')) {
        panic("Unknown file type!");
    }
    struct ProgHeader *p_header =
        (struct ProgHeader *)((usize)elf + e_header->e_phoff);
    // 遍历所有的程序段
    for (int i = 0; i < e_header->e_phnum; ++i, ++p_header) {
        if (p_header->p_type != ELF_PROG_LOAD) {
            continue;
        }
        usize flags = convert_flags(p_header->p_flags);
        usize start_va = p_header->p_vaddr,
              end_va = start_va + p_header->p_memsz;
        if (start_va == end_va) {
            continue;
        }
        struct Segment *segment = new_segment(start_va, end_va, flags, Framed);
        char *data = (char *)((usize)elf + p_header->p_offset);
        map_segment(res->root_ppn, segment, data, p_header->p_memsz);
        list_add(&segment->list, &res->segment_list);
    }
    return res;
}