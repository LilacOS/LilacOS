#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include "kernel/types.h"
#include "kernel/fs.h"

/* 该程序用于将 rootfs 打包成一个 SimpleFS 镜像文件 */
// ---------------------------------------------------
// |            |         |            |             |
// | superblock | freemap |    FAT     | other block |
// |            |         |            |             |
// ---------------------------------------------------

// 总块数 2048 块，大小为 1M
#define IMG_SIZE (BLOCK_SIZE * BLOCK_NUM)
#define MIN(a, b) ((a) < (b) ? (a) : (b))

char *Image;
int unused_blocks = BLOCK_NUM;

static inline void *get_block(int block) {
    return (void *)Image + block * BLOCK_SIZE;
}

static inline struct Inode *get_inode(int idx) {
    return (struct Inode *)get_block(idx);
}

/**
 * 分配空闲块
 */
int alloc_free_block() {
    int *freemap = (int *)get_block(1);
    for (int i = 0; i < BLOCK_NUM; ++i) {
        int index = i / 32;
        int offset = i % 32;
        if ((freemap[index] & (1 << offset)) == 0) {
            // 该块未被分配，进行分配
            freemap[index] |= (1 << offset);
            --unused_blocks;
            return i;
        }
    }
    return -1;
}

/**
 * 分配 inode 节点并填充 FAT 表
 *
 * @return -1 分配失败
 */
int alloc_inode() {
    uint16 *fat = (uint16 *)get_block(2);
    for (int i = 0; i < BLOCK_SIZE / sizeof(uint16); ++i) {
        if (!fat[i]) {
            int inode = alloc_free_block();
            if (inode != -1) {
                fat[i] = inode;
            }
            return inode;
        }
    }
    return -1;
}

/**
 * 将磁盘块号填入 inode 节点空闲的位置
 */
void put_block(int block, struct Inode *inode) {
    for (int i = 0; i < 12; ++i) {
        if (!inode->direct[i]) {
            inode->direct[i] = block;
            return;
        }
    }
    if (!inode->indirect) {
        inode->indirect = alloc_free_block();
    }
    uint32 *indirect = (uint32 *)get_block(inode->indirect);
    for (int i = 0; i < BLOCK_SIZE / sizeof(uint32); ++i) {
        if (!indirect[i]) {
            indirect[i] = block;
            return;
        }
    }
}

/**
 * 遍历文件夹，填充文件 inode 节点表
 *
 * @param path 当前文件夹路径
 */
void walk(char *path) {
    // 打开当前文件夹
    DIR *dir = opendir(path);

    // 遍历当前文件夹下所有文件
    struct dirent *entry;
    int new_idx;
    struct Inode *new_inode;
    while ((entry = readdir(dir))) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }
        if (entry->d_type == DT_REG) {
            // 普通文件处理
            new_idx = alloc_inode();
            new_inode = get_inode(new_idx);
            strcpy(new_inode->filename, entry->d_name);
            // 获取文件信息
            char new_path[256];
            sprintf(new_path, "%s/%s", path, entry->d_name);
            struct stat buf;
            stat(new_path, &buf);
            new_inode->size = buf.st_size;
            new_inode->blocks = (new_inode->size - 1) / BLOCK_SIZE + 1;

            // 复制文件数据
            int size = 0;
            FILE *fp = fopen(new_path, "rb");
            while (size < buf.st_size) {
                int block = alloc_free_block();
                char *data = (char *)get_block(block);
                int len = MIN(buf.st_size - size, BLOCK_SIZE);
                fread(data, len, 1, fp);
                size += len;
                put_block(block, new_inode);
            }
            fclose(fp);
        } else {
            continue;
        }
    }
    closedir(dir);
}

void main() {
    Image = (char *)malloc(IMG_SIZE);
    memset(Image, 0, IMG_SIZE);

    // 设置超级块、空闲块位图、FAT 表所在磁盘块已被分配
    if ((alloc_free_block() != 0) || (alloc_free_block() != 1) ||
        (alloc_free_block() != 2)) {
        printf("Error!");
        exit(1);
    }

    // 遍历根目录，并设置和填充数据
    walk("rootfs");

    // 填充超级块信息
    struct SuperBlock *sb = (struct SuperBlock *)get_block(0);
    sb->magic = MAGIC_NUM;
    sb->blocks = BLOCK_NUM;
    sb->unused_blocks = unused_blocks;

    // 将 Image 写到磁盘上
    FILE *img = fopen("fs.img", "w+b");
    fwrite(Image, IMG_SIZE, 1, img);
    fflush(img);
    fclose(img);

    free(Image);
}