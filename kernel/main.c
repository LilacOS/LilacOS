#include "types.h"
#include "def.h"

asm(".include \"kernel/entry.asm\"");

extern void sbss();
extern void ebss();

void testAlloc()
{
    printf("alloc %p\n", allocFrame());
    usize f = allocFrame();
    printf("alloc %p\n", f);
    printf("alloc %p\n", allocFrame());
    printf("dealloc %p\n", f);
    deallocFrame(f);
    printf("alloc %p\n", allocFrame());
    printf("alloc %p\n", allocFrame());
}

void main()
{
    /* 初始化 .bss 段 */
    uint64 *bss_start_init = (uint64 *)sbss, *bss_end_init = (uint64 *)ebss;
    for (volatile uint64 *bss_mem = bss_start_init; bss_mem < bss_end_init; ++bss_mem)
    {
        *bss_mem = 0;
    }

    init_interrupt();
    extern void initMemory();
    initMemory();
    testAlloc();
    shutdown();
}