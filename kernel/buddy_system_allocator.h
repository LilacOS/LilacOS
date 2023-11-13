#ifndef __BUDDY_SYSTEM_ALLOCATOR_H__
#define __BUDDY_SYSTEM_ALLOCATOR_H__

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
// PUSH(struct Buddy*, uint64*, int);
#define PUSH(buddy, block, order)                       \
    ({                                                  \
        *((block)) = (uint64)buddy->free_list[(order)]; \
        buddy->free_list[(order)] = (block);            \
    })
// uint64* POP(struct Buddy*, int);
#define POP(buddy, order)                               \
    ({                                                  \
        uint64 *tmp = (buddy)->free_list[(order)];      \
        (buddy)->free_list[(order)] = (uint64 *)(*tmp); \
        tmp;                                            \
    })

#define MAX_ORDER 30
struct Buddy
{
    uint64 *free_list[MAX_ORDER];
    uint64 total;
    uint64 allocated;
};

#endif