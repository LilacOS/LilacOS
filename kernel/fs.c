#include "types.h"
#include "def.h"
#include "fs.h"
#include "string.h"
#include "process.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

extern struct ProcessControlBlock *current;

struct Inode *ROOT_INODE;

static inline void *get_block(int block) {
    extern void _fs_img_start();
    return (void *)_fs_img_start + (block * BLOCK_SIZE);
}

/**
 * 分配空闲块
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
}

void init_fs() {
    struct SuperBlock *sb = get_block(0);
    ROOT_INODE = (struct Inode *)get_block(1 + sb->freemap_blocks);
    printf("***** Init FS *****\n");
}

/**
 * 在当前目录下查找文件
 *
 * - 若 path 为相对路径且 inode 不为空，则以 inode 为当前目录查找
 * - 若 path 为相对路径且 inode 为空，则以根目录为当前目录查找
 * - 若 path 为绝对路径，则忽略 inode
 */
struct Inode *lookup(struct Inode *inode, char *path) {
    if (path[0] == '\0')
        return inode;
    if (path[0] == '/') {
        inode = ROOT_INODE;
        ++path;
    }
    if (!inode)
        inode = ROOT_INODE;
    if (inode->type != TYPE_DIR)
        return NULL;

    // 解析第一个文件名
    char name[strlen(path) + 1];
    int i = 0;
    for (; *path != '/' && *path != '\0'; ++path, ++i) {
        name[i] = *path;
    }
    name[i] = '\0';
    if (*path == '/')
        ++path;

    if (!strcmp(".", name)) {
        return lookup(inode, path);
    }
    if (!strcmp("..", name)) {
        struct Inode *parent = (struct Inode *)get_block(inode->direct[1]);
        return lookup(parent, path);
    }

    uint32 *indirect = (uint32 *)get_block(inode->indirect);
    for (int i = 2; i < inode->blocks; ++i) {
        struct Inode *tmp = i < 12
                                ? (struct Inode *)get_block(inode->direct[i])
                                : (struct Inode *)get_block(indirect[i - 12]);
        if (!strcmp((char *)tmp->filename, name)) {
            return lookup(tmp, path);
        }
    }
    return NULL;
}

/**
 * 将文件的数据读取到 buf 中
 */
int readall(struct Inode *inode, char *buf) {
    if (inode->type != TYPE_FILE) {
        panic("Cannot read a directory!\n");
    }

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

struct Inode *get_dir(char *path) { return NULL; }

/**
 * 创建普通文件
*/
struct Inode *create(char *path) {
    struct Inode *dir = get_dir(path);
    if (dir->blocks == 12 + BLOCK_SIZE / sizeof(uint32)) {
        // 目录文件夹容量已满
        return NULL;
    }

    // 分配普通文件 inode 节点
    int block = alloc_free_block();
    struct Inode *inode = (struct Inode *)get_block(block);
    inode->size = inode->blocks = 0;
    inode->type = TYPE_FILE;
    for (int i = 0; i < 12; ++i) {
        inode->direct[i] = 0;
    }
    inode->indirect = 0;

    for (int i = 2; i < 12 && i < dir->blocks; ++i) {
        if (!(dir->direct[i])) {
            dir->direct[i] = block;
            break;
        }
    }
    uint32 *indirect = (uint32 *)get_block(dir->indirect);
    for (int i = 0; i + 12 < dir->blocks; ++i) {
        if (!(indirect[i])) {
            indirect[i] = block;
            break;
        }
    }

    return inode;
}

int sys_open(char *path, int flags) {
    for (int i = 0; i < NR_OPEN; ++i) {
        if (!(current->files[i])) {
            struct Inode *inode = lookup(NULL, path);
            if (!inode && (flags & O_CREATE)) {
                inode = create(path);
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