#include "kernel/types.h"
#include "ulib.h"

int main() {
    int pid;

    // 简单测试文件系统功能
    pid = fork();
    if (!pid) {
        exec("filetest\0");
    } else {
        pid = wait();
        printf("Child %d terminated!\n", pid);
    }

    pid = fork();
    if (!pid) {
        exec("shell\0");
    }
    while ((pid = wait()) != -1) {
        printf("Child %d terminated!\n", pid);
    }
    return 0;
}