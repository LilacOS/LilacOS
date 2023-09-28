#include "types.h"
#include "def.h"

extern void sbss();
extern void ebss();

void
main()
{
    /* 初始化 .bss 段 */
    uint64 *bss_start_init = (uint64 *) sbss, *bss_end_init = (uint64 *) ebss;
    for (volatile uint64 *bss_mem = bss_start_init; bss_mem < bss_end_init; ++bss_mem) {
        *bss_mem = 0;
    }
    printf("Hello from Moonix!\n");
    panic("Nothing to do!");
}