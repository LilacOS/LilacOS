#include "types.h"
#include "def.h"
#include "fs.h"
#include "string.h"
#include "process.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

extern struct ProcessControlBlock *current;

static inline void *get_block(int block) {
    extern void _fs_img_start();
    return (void *)_fs_img_start + (block * BLOCK_SIZE);
}

static inline struct Inode *get_inode(int idx) {
    return (struct Inode *)get_block(idx);
}

/**
 * 分配空闲块
 *
 * @return -1 分配失败
 */
int alloc_free_block() {
    struct SuperBlock *sb = get_block(0);
    if (sb->unused_blocks) {
        --(sb->unused_blocks);
    } else {
        return -1;
    }
    int *freemap = (int *)get_block(1);
    for (int i = 0; i < BLOCK_NUM; ++i) {
        int index = i / 32;
        int offset = i % 32;
        if ((freemap[index] & (1 << offset)) == 0) {
            // 该块未被分配，进行分配
            freemap[index] |= (1 << offset);
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

void init_fs() { printf("***** Init FS *****\n"); }

/**
 * 查找文件
 */
struct Inode *lookup(char *name) {
    uint16 *fat = (uint16 *)get_block(2);
    for (int i = 0; i < BLOCK_SIZE / sizeof(uint16); ++i) {
        if (fat[i]) {
            struct Inode *inode = get_inode(fat[i]);
            if (!strcmp((char *)(inode->filename), name)) {
                return inode;
            }
        }
    }
    return NULL;
}

/**
 * 将文件的数据读取到 buf 中
 */
int readall(struct Inode *inode, char *buf) {
    int size = 0;
    uint32 *indirect = (uint32 *)get_block(inode->indirect);
    for (int i = 0; size < inode->size; ++i) {
        char *src = i < 12 ? (char *)get_block(inode->direct[i])
                           : (char *)get_block(indirect[i - 12]);
        int len = MIN(inode->size - size, BLOCK_SIZE);
        for (int j = 0; j < len; ++j) {
            buf[j] = src[j];
        }
        buf += len;
        size += len;
    }
    return size;
}

/**
 * 创建普通文件
 */
struct Inode *create(char *name) {
    int block = alloc_free_block();
    struct Inode *inode = get_inode(block);
    inode->size = inode->blocks = 0;
    for (int i = 0; i < 12; ++i) {
        inode->direct[i] = 0;
    }
    inode->indirect = 0;
    return inode;
}

int sys_open(char *name, int flags) {
    for (int i = 0; i < NR_OPEN; ++i) {
        if (!(current->files[i])) {
            struct Inode *inode = lookup(name);
            if (!inode && (flags & O_CREATE)) {
                inode = create(name);
            }
            struct File *file = (struct File *)alloc(sizeof(struct File));
            file->count = 1;
            file->off = 0;
            file->inode = inode;
            current->files[i] = file;
            return i;
        }
    }
    return -1;
}

int sys_close(int fd) {
    if (fd >= 0 && fd < NR_OPEN && current->files[fd]) {
        struct File *file = current->files[fd];
        if (!--(file->count)) {
            dealloc((void *)file->inode, sizeof(struct Inode));
            dealloc((void *)file, sizeof(struct File));
        }
        current->files[fd] = NULL;
    }
    return 0;
}

int sys_read(int fd, char *buf, int count) {
    int res = 0;
    if (fd >= 0 && fd < NR_OPEN && current->files[fd]) {
    }
    return res;
}

int sys_write() { return 0; }

void dealloc_files(struct File **files) {
    for (int i = 0; i < NR_OPEN; ++i) {
        if (files[i]) {
            if (!--(files[i]->count)) {
                dealloc((void *)files[i]->inode, sizeof(struct Inode));
                dealloc((void *)files[i], sizeof(struct File));
            }
            files[i] = NULL;
        }
    }
}