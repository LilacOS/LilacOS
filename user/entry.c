#include "types.h"
#include "ulib.h"
#include "syscall.h"

__attribute__((weak)) int main()
{
    panic("No main linked!\n");
    return 0;
}

void _start(uint8 _args, uint8 *_argv)
{
    exit(main());
}