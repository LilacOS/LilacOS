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

    for (int i = 0; i < 5; ++i) {
        pid = fork();
        if (!pid) {
            exec("hello\0");
        } else {
            printf("Hello world from parent, pid = %d\n", getpid());
        }
    }
    while ((pid = wait()) != -1) {
        printf("Child %d terminated!\n", pid);
    }
    return 0;
}