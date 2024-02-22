#include "types.h"
#include "def.h"
#include "consts.h"

void test_alloc() {
    void *blocks[10];
    printf("Buddy test\n");
    int i = 0;
    printf("step0\n");
    for (int j = 0; j < 10; ++j) {
        blocks[i++] = alloc(30);
        printf("%p\n", blocks[i - 1]);
    }

    printf("step1\n");
    dealloc(blocks[--i], 64);
    blocks[i++] = alloc(64);
    printf("%p\n", blocks[i - 1]);

    printf("step2\n");
    i = 0;
    for (int j = 0; j < 10; ++j) {
        dealloc(blocks[j], 30);
    }
    for (int j = 0; j < 10; ++j) {
        blocks[i++] = alloc(64);
        printf("%p\n", blocks[i - 1]);
    }
    for (int j = 0; j < 10; ++j) {
        dealloc(blocks[j], 30);
    }
    printf("Buddy test passed!\n");
}

void main() {
    /* 初始化 .bss 段 */
    uint64 *bss_start_init = (uint64 *)sbss, *bss_end_init = (uint64 *)ebss;
    for (volatile uint64 *bss_mem = bss_start_init; bss_mem < bss_end_init;
         ++bss_mem) {
        *bss_mem = 0;
    }

    init_trap();
    init_memory();
    test_alloc();
    remap_kernel();
    shutdown();
}