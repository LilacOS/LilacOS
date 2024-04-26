#ifndef _SIMPLEFS_H
#define _SIMPLEFS_H

#include "types.h"

#define BLOCK_SIZE 512
#define BLOCK_NUM 2048
#define MAGIC_NUM 0x4D534653U // MSFS

#define O_CREATE 0x200

struct SuperBlock {
    uint32 magic;         // 魔数
    uint16 blocks;        // 总磁盘块数
    uint16 unused_blocks; // 未使用的磁盘块数
};

struct Inode {
    uint32 size;        // 文件大小
    uint8 filename[32]; // 文件名称
    uint16 blocks;      // 占据磁盘块个数
    uint16 direct[12];  // 直接磁盘块
    uint16 indirect;    // 间接磁盘块
};

struct File {
    int count;
    int off;
    struct Inode *inode;
};

#endif