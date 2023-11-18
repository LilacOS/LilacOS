#ifndef _ELF_H
#define _ELF_H

#include "types.h"

// ELF 魔数
#define ELF_MAGIC 0x464C457FU

typedef struct
{
	uint magic;
	uchar elf[12];
	ushort type;
	ushort machine;
	uint version;
	uint64 entry;
	uint64 phoff;
	uint64 shoff;
	uint flags;
	ushort ehsize;
	ushort phentsize;
	ushort phnum;
	ushort shentsize;
	ushort shnum;
	ushort shstrndx;
} ElfHeader;

typedef struct
{
	uint32 type;
	uint32 flags;
	uint64 off;
	uint64 vaddr;
	uint64 paddr;
	uint64 filesz;
	uint64 memsz;
	uint64 align;
} ProgHeader;

// 程序段头类型
#define ELF_PROG_LOAD 1

// 程序段头的权限
#define ELF_PROG_FLAG_EXEC 1
#define ELF_PROG_FLAG_WRITE 2
#define ELF_PROG_FLAG_READ 4

#endif