#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/string.h"
#include "ulib.h"

int main() {
    // 打开文件
    int fd = open("test.txt", O_CREATE);
    if (fd == -1) {
        printf("create file failed!\n");
        exit();
    }
    printf("open file, fd = %d\n", fd);
    // 写入数据
    char *data = "Hello, file system!";
    ssize_t bytes_written = write(fd, data, strlen(data));
    if (bytes_written == -1) {
        printf("write data failed!\n");
        exit();
    }
    printf("write data: %d\n", bytes_written);
    close(fd);

    // 重新打开文件以读取数据
    fd = open("test.txt", 0);
    if (fd == -1) {
        printf("open file failed!\n");
        exit();
    }
    printf("open file, fd = %d\n", fd);
    // 读取数据
    char buffer[100];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        printf("read data failed!\n");
        exit();
    }
    printf("read data: %d\n", bytes_read);
    // 添加字符串结束符
    buffer[bytes_read] = '\0';
    printf("read data: %s\n", buffer);
    close(fd);

    printf("File test passed!\n");
    return 0;
}