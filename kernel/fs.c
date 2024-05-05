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
 * 分配 inode 节点号并填充 FAT 表
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
 * 从文件偏移 `off` 处读取至多 `count` 字节数据到 `buf` 中
 *
 * @return 最终读取的字节数量
 */
int read_from_inode(struct Inode *inode, int off, char *buf, int count) {
    // 计算最多能够读取的字节数
    int max_size = MIN(inode->size - off, count);
    if (max_size <= 0) {
        return 0;
    }
    int num = 0;
    int start_block = off / BLOCK_SIZE;
    int block_off = off % BLOCK_SIZE;
    // 前面判断了文件大小，即使间接磁盘块未分配也不要紧
    uint32 *indirect = (uint32 *)get_block(inode->indirect);
    for (int i = start_block; num < max_size; ++i) {
        char *src = i < 12 ? (char *)get_block(inode->direct[i])
                           : (char *)get_block(indirect[i - 12]);
        src += block_off;
        int len = MIN(max_size - num, BLOCK_SIZE - block_off);
        for (int j = 0; j < len; ++j) {
            buf[j] = src[j];
        }
        buf += len;
        num += len;
        block_off = 0;
    }
    return num;
}

/**
 * 将文件的全部数据读取到 `buf` 中
 */
int readall(struct Inode *inode, char *buf) {
    return read_from_inode(inode, 0, buf, inode->size);
}

/**
 * 创建普通文件
 */
struct Inode *create(char *name) {
    int idx = alloc_inode();
    struct Inode *inode = get_inode(idx);
    inode->size = inode->blocks = 0;
    strcpy((char *)(inode->filename), name);
    for (int i = 0; i < 12; ++i) {
        inode->direct[i] = 0;
    }
    inode->indirect = 0;
    return inode;
}

int sys_open(char *name, int flags) {
    for (int i = 0; i < NR_OPEN; ++i) {
        if (!(current->files[i])) {
            struct Inode *inode;
            inode = lookup(name);
            if (!inode) {
                if ((flags & O_CREATE)) {
                    inode = create(name);
                } else {
                    return -1;
                }
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
            dealloc((void *)file, sizeof(struct File));
        }
        current->files[fd] = NULL;
    }
    return 0;
}

int sys_read(int fd, char *buf, int count) {
    if (!count) {
        return 0;
    }
    if (fd >= 0 && fd < NR_OPEN && current->files[fd]) {
        struct File *file = current->files[fd];
        if (file->type == FILE_STDIO) {
            // 标准输入
            while (1) {
                char c = console_getchar();
                if (c == (char)-1) {
                    yield();
                } else {
                    buf[0] = c;
                    return 1;
                }
            }
        }
        // 文件输入
        int num = read_from_inode(file->inode, file->off, buf, count);
        file->off += num;
        return num;
    }
    return -1;
}

/**
 * 扩充文件空间到至少 `new_size`
 */
void increase_size(struct Inode *inode, int new_size) {
    if (inode->blocks * BLOCK_SIZE >= new_size) {
        return;
    }
    // 计算还需分配几个磁盘块
    int num = (new_size + BLOCK_SIZE - 1) / BLOCK_SIZE - inode->blocks;
    inode->blocks += num;
    // 分配直接磁盘块
    for (int i = 0; i < 12 && num; ++i) {
        if (!inode->direct[i]) {
            inode->direct[i] = alloc_free_block();
            --num;
        }
    }
    if (!num) {
        return;
    }
    // 分配间接磁盘块
    if (!inode->indirect) {
        inode->indirect = alloc_free_block();
    }
    uint32 *indirect = (uint32 *)get_block(inode->indirect);
    for (int i = 0; i < BLOCK_SIZE / sizeof(uint32) && num; ++i) {
        if (!indirect[i]) {
            indirect[i] = alloc_free_block();
            --num;
        }
    }
}

int sys_write(int fd, char *buf, int count) {
    if (fd >= 0 && fd < NR_OPEN && current->files[fd]) {
        struct File *file = current->files[fd];

        if (file->type == FILE_STDIO) {
            // 标准输出
            for (int i = 0; i < count; ++i) {
                console_putchar(buf[i]);
            }
            return count;
        }

        // 文件输出
        struct Inode *inode = file->inode;
        int new_size = file->off + count;
        increase_size(file->inode, new_size);

        int num = 0;
        int start_block = file->off / BLOCK_SIZE;
        int block_off = file->off % BLOCK_SIZE;
        // 前面扩充了文件大小，即使间接磁盘块未分配也不要紧
        uint32 *indirect = (uint32 *)get_block(inode->indirect);
        for (int i = start_block; num < count; ++i) {
            char *dst = i < 12 ? (char *)get_block(inode->direct[i])
                               : (char *)get_block(indirect[i - 12]);
            dst += block_off;
            int len = MIN(count - num, BLOCK_SIZE - block_off);
            for (int j = 0; j < len; ++j) {
                dst[j] = buf[j];
            }
            buf += len;
            num += len;
            block_off = 0;
        }
        // 更新文件偏移量和文件大小
        file->off += num;
        if (inode->size < file->off) {
            inode->size = file->off;
        }
        return num;
    }
    return -1;
}

void dealloc_files(struct File **files) {
    for (int i = 0; i < NR_OPEN; ++i) {
        if (files[i]) {
            if (!--(files[i]->count)) {
                dealloc((void *)files[i], sizeof(struct File));
            }
            files[i] = NULL;
        }
    }
}