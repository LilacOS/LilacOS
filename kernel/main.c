#include "types.h"
#include "def.h"
#include "consts.h"

inline void *wrap_alloc(usize size) {
    extern uint64 next_power_of_two(uint64);
    uint64 adjust_size = size <= 8 ? 8 : next_power_of_two(size);
    void *tmp = alloc(size);
    printf("request %dB, actually alloc 0x%xB, start address: %p\n", size,
           adjust_size, tmp);
    return tmp;
}

void test_alloc() {
    void *blocks[10];
    printf("Buddy test\n");

    printf("step0\n");
    for (int j = 0; j < 10; ++j) {
        blocks[j] = wrap_alloc(1);
    }
    for (int j = 0; j < 10; ++j) {
        dealloc(blocks[j], 1);
    }

    printf("step1\n");
    for (int j = 0; j < 10; ++j) {
        blocks[j] = wrap_alloc(30);
    }
    for (int j = 0; j < 10; ++j) {
        dealloc(blocks[j], 30);
    }

    printf("step2\n");
    blocks[0] = wrap_alloc(100);
    blocks[1] = wrap_alloc(60);
    blocks[2] = wrap_alloc(100);
    dealloc(blocks[0], 100);
    blocks[3] = wrap_alloc(30);
    dealloc(blocks[1], 60);
    dealloc(blocks[3], 30);
    dealloc(blocks[2], 100);
    printf("Buddy test passed!\n");
}

void main() {
    /* 初始化 .bss 段 */
    uint64 *bss_start_init = (uint64 *)sbss, *bss_end_init = (uint64 *)ebss;
    for (volatile uint64 *bss_mem = bss_start_init; bss_mem < bss_end_init;
         ++bss_mem) {
        *bss_mem = 0;
    }

    init_memory();
    test_alloc();
    init_fs();
    init_trap();
    init_process();
    shutdown();
}