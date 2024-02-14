#include "def.h"
#include "consts.h"
#include "buddy_system_allocator.h"
#include "mapping.h"

static struct Buddy allocator;

void init_allocator() {
    init_buddy(&allocator);
    add_to_buddy(&allocator, (void *)ekernel,
                 (void *)(MEMORY_END + KERNEL_MAP_OFFSET));
}

/**
 * 分配内存
 *
 * @param size 分配内存的大小，会被调整到最近的2次幂，且最小为64位
 * @return 内存块的起始地址
 * @exception 内存不够将会panic
 */
void *alloc(usize size) {
    void *block = buddy_alloc(&allocator, size);
    if (block == NULL) {
        panic("Not enough memory!");
    }
    return block;
}

/**
 * 释放内存
 *
 * @param block 释放的内存块
 * @param size block的大小
 */
void dealloc(void *block, usize size) {
    buddy_dealloc(&allocator, block, size);
}

/**
 * 分配一个物理页帧
 *
 * @return 物理页帧号
 */
usize alloc_frame() {
    char *page = (char *)alloc(PAGE_SIZE);
    for (int i = 0; i < PAGE_SIZE; ++i) {
        page[i] = 0;
    }
    return __pa((usize)page) >> 12;
}

/**
 * 释放页帧
 *
 * @param ppm 物理页帧号
 */
void dealloc_frame(usize ppn) { dealloc((void *)__va(ppn << 12), PAGE_SIZE); }

void init_memory() {
    init_allocator();
    printf("***** Init Memory *****\n");
}