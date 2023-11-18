#include "types.h"
#include "def.h"
#include "context.h"
#include "syscall.h"

usize syscall(usize id, usize args[3], TrapContext *context)
{
    switch (id)
    {
    case SYS_exit:
        exit_current();
    case SYS_putchar:
        console_putchar(args[0]);
        return 0;
    default:
        printf("Unknown syscall id %d\n", id);
        panic("");
    }
}