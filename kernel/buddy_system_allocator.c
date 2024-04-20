#include "def.h"
#include "types.h"
#include "buddy_system_allocator.h"

uint64 next_power_of_two(uint64 size);
uint64 prev_power_of_two(uint64 size);
int get_order(uint64 size);

void init_buddy(struct Buddy *buddy) {
    for (int i = 0; i < MAX_ORDER; ++i) {
        buddy->free_list[i] = NULL;
    }
    buddy->allocated = 0;
    buddy->total = 0;
}

/**
 * 添加内存区域 [start, end) 到分配器
 *
 * @param buddy 伙伴系统分配器
 * @param start 内存区域的起始地址
 * @param end 内存区域结束地址
 */
void add_to_buddy(struct Buddy *buddy, void *start, void *end) {
    uint64 *current = start;
    while ((uint64)current < (uint64)end) {
        uint64 lowbit = (uint64)current & (~(uint64)current + 1);
        uint64 size =
            MIN(lowbit, MIN(prev_power_of_two((uint64)end - (uint64)current),
                            1 << MAX_ORDER));
        int order = get_order(size);
        // 插入链表
        PUSH(buddy, current, order);
        current = (uint64 *)((uint64)current + size);
        buddy->total += size;
    }
}

/**
 * 分配内存块
 *
 * @param buddy 伙伴系统分配器
 * @param size 分配内存的大小，会被调整为2的幂次方
 * @return 内存块的起始地址
 */
void *buddy_alloc(struct Buddy *buddy, uint64 size) {
    uint64 adjust_size = MAX(next_power_of_two(size), 64);
    int order = get_order(adjust_size);
    for (int i = order; i < MAX_ORDER; ++i) {
        if (buddy->free_list[i]) {
            for (int j = i; j > order; --j) {
                uint64 *block = POP(buddy, j);
                PUSH(buddy, (uint64 *)((uint64)block + (1 << (j - 1))), j - 1);
                PUSH(buddy, block, j - 1);
            }
            buddy->allocated += adjust_size;
            return (void *)(POP(buddy, order));
        }
    }
    return NULL;
}

/**
 * 释放内存块
 *
 * @param buddy 伙伴系统分配器
 * @param block 释放内存块的起始地址
 * @param size 内存块的大小，会被调整为2的幂次方
 */
void buddy_dealloc(struct Buddy *buddy, void *block, uint64 size) {
    uint64 adjust_size = MAX(next_power_of_two(size), 64);
    int order = get_order(adjust_size);
    PUSH(buddy, (uint64 *)block, order);
    uint64 *current_block = block;
    int current_order = order;
    while (current_order < MAX_ORDER) {
        uint64 *buddy_block =
            (uint64 *)((uint64)current_block ^ (1 << current_order));
        int flag = 0;
        for (uint64 *prev = buddy->free_list[current_order],
                    *next = (uint64 *)*prev;
             next != NULL; prev = next, next = (uint64 *)(*next)) {
            if (next == buddy_block) {
                *prev = *next;
                flag = 1;
                break;
            }
        }
        if (flag) {
            POP(buddy, current_order);
            current_block = MIN(current_block, buddy_block);
            current_order += 1;
            PUSH(buddy, current_block, current_order);
        } else {
            break;
        }
    }
    buddy->allocated -= adjust_size;
}

/**
 * 获得大于等于 size 的最小的 2 的幂级数
 * 算法来自于 Java 的 Hashmap
 */
uint64 next_power_of_two(uint64 size) {
    uint64 n = size - 1;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

/**
 * 获得小于等于 size 的最大的 2 的幂级数
 */
uint64 prev_power_of_two(uint64 size) {
    uint64 next = next_power_of_two(size);
    return size == next ? size : next >> 1;
}

/**
 * log2(size)
 */
int get_order(uint64 size) {
    int order = 0;
    while (size > 1) {
        size >>= 1;
        ++order;
    }
    return order;
}