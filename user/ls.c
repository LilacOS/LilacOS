#include "kernel/types.h"
#include "kernel/fs.h"
#include "ulib.h"

int main() {
    int fd = open("/\0", 0);
    if (fd == -1) {
        printf("open file failed!\n");
        exit();
    }
    struct Inode inode;
    while (read(fd, (char *)(&inode), sizeof(struct Inode)) != 0) {
        printf("%s ", inode.filename);
    }
    printf("\n");
    return 0;
}