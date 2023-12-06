#include "types.h"
#include "def.h"
#include "fs.h"
#include "string.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct Inode *ROOT_INODE;

static inline void *get_block(int block)
{
    extern void _fs_img_start();
    return (void *)_fs_img_start + (block * BLOCK_SIZE);
}

void init_fs()
{
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
struct Inode *lookup(struct Inode *inode, char *path)
{
    if (path[0] == '\0')
        return inode;
    if (path[0] == '/')
    {
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
    for (; *path != '/' && *path != '\0'; ++path, ++i)
    {
        name[i] = *path;
    }
    name[i] = '\0';
    if (*path == '/')
        ++path;

    if (!strcmp(".", name))
    {
        return lookup(inode, path);
    }
    if (!strcmp("..", name))
    {
        struct Inode *parent = (struct Inode *)get_block(inode->direct[1]);
        return lookup(parent, path);
    }

    uint32 *indirect = (uint32 *)get_block(inode->indirect);
    for (int i = 2; i < inode->blocks; ++i)
    {
        struct Inode *tmp = i < 12 ? (struct Inode *)get_block(inode->direct[i])
                                   : (struct Inode *)get_block(indirect[i - 12]);
        if (!strcmp((char *)tmp->filename, name))
        {
            return lookup(tmp, path);
        }
    }
    return NULL;
}

/**
 * 将文件 inode 的数据读取到 buf 中
 */
int readall(struct Inode *inode, char *buf)
{
    if (inode->type != TYPE_FILE)
    {
        panic("Cannot read a directory!\n");
    }

    int size = 0;
    uint32 *indirect = (uint32 *)get_block(inode->indirect);
    for (int i = 0; size < inode->size; ++i)
    {
        char *src = i < 12 ? (char *)get_block(inode->direct[i])
                           : (char *)get_block(indirect[i - 12]);
        int len = MIN(inode->size - size, BLOCK_SIZE);
        for (int j = 0; j < len; ++j)
        {
            buf[j] = src[j];
        }
        buf += len;
        size += len;
    }
    return size;
}