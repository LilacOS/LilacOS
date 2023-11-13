#include "types.h"
#include "def.h"
#include "riscv.h"

static const usize CLOCK_FREQ = 10000000;
// 每10ms触发一次时钟中断
static const usize TICKS_PER_SEC = 100;

void set_next_timeout();

void init_timer()
{
    // 时钟中断使能
    w_sie(r_sie() | SIE_STIE);
    // 设置第一次时钟中断
    set_next_timeout();
}

/**
 * 设置下一次时钟
 */
void set_next_timeout()
{
    set_timer(r_time() + CLOCK_FREQ / TICKS_PER_SEC);
}