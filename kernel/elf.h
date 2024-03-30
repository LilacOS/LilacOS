#ifndef _ELF_H
#define _ELF_H

#include "types.h"

#define EI_NIDENT 16

struct ElfHeader {
    uchar e_ident[EI_NIDENT];
    ushort e_type;
    ushort e_machine;
    uint e_version;
    uint64 e_entry;
    uint64 e_phoff;
    uint64 e_shoff;
    uint e_flags;
    ushort e_ehsize;
    ushort e_phentsize;
    ushort e_phnum;
    ushort e_shentsize;
    ushort e_shnum;
    ushort e_shstrndx;
};

struct ProgHeader {
    uint32 p_type;
    uint32 p_flags;
    uint64 p_offset;
    uint64 p_vaddr;
    uint64 p_paddr;
    uint64 p_filesz;
    uint64 p_memsz;
    uint64 p_align;
};

// 程序段头类型
#define ELF_PROG_LOAD 1

// 程序段头的权限
#define ELF_PROG_FLAG_EXEC 1
#define ELF_PROG_FLAG_WRITE 2
#define ELF_PROG_FLAG_READ 4

#endif