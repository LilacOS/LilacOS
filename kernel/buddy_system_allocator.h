#ifndef __BUDDY_SYSTEM_ALLOCATOR_H__
#define __BUDDY_SYSTEM_ALLOCATOR_H__

#define MAX_ORDER 30
struct Buddy
{
    uint64 *free_list[MAX_ORDER];
    uint64 total;
    uint64 allocated;
};
void init_buddy(struct Buddy *buddy);
void add_to_buddy(struct Buddy *buddy, void *start, void *end);
void *buddy_alloc(struct Buddy *buddy, uint64 num);
void buddy_dealloc(struct Buddy *buddy, void *block, uint64 num);

uint64 next_power_of_two(uint64 size);
uint64 prev_power_of_two(uint64 size);
int get_order(uint64 size);

#endif