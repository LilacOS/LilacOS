#include "kernel/types.h"
#include "ulib.h"

int main() {
    int pid = getpid();
    for (int i = 0; i < 5; ++i) {
        printf("Hello world from user mode program, pid = %d\n", pid);
    }
    return 0;
}