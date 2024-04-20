#ifndef _SIMPLEFS_H
#define _SIMPLEFS_H

#include "types.h"

#define BLOCK_SIZE 512
#define MAGIC_NUM 0x4D534653U // MSFS

#define TYPE_FILE 0
#define TYPE_DIR 1

struct SuperBlock {
    uint32 magic;          // 魔数
    uint32 blocks;         // 总磁盘块数
    uint32 unused_blocks;  // 未使用的磁盘块数
    uint32 freemap_blocks; // 空闲位图所占块数
};

struct Inode {
    uint32 size;        // 文件大小，type 为文件夹时该字段为 0
    uint32 type;        // 文件类型
    uint8 filename[32]; // 文件名称
    uint32 blocks;      // 占据磁盘块个数
    uint32 direct[12];  // 直接磁盘块
    uint32 indirect;    // 间接磁盘块
};

#endif