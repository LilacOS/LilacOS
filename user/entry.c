#include "types.h"
#include "ulib.h"
#include "syscall.h"

__attribute__((weak)) int main()
{
    panic("No main linked!\n");
    return 0;
}

void _start()
{
    main();
    exit();
}