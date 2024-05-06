#include "kernel/types.h"
#include "ulib.h"

#define LF 0x0au
#define CR 0x0du
#define BS 0x08u
#define DL 0x7fu

char line[256];
int len;

inline int is_empty() { return len == 0; }

inline void clear() {
    for (int i = 0; i < 256; ++i) {
        line[i] = '\0';
    }
    len = 0;
}

int main() {
    clear();
    printf("$ ");
    while (1) {
        int pid;
        char c = getchar();
        switch (c) {
        case LF:
        case CR:
            printf("\n");
            if (!is_empty()) {
                pid = fork();
                if (!pid) {
                    if (exec(line) == -1) {
                        printf("%s: No such file\n", line);
                        exit();
                    }
                }
                while (wait() == -1) {
                }
                clear();
            }
            printf("$ ");
            break;
        case BS:
        case DL:
            if (!is_empty()) {
                putchar(BS);
                putchar(' ');
                putchar(BS);
                line[--len] = '\0';
            }
            break;
        default:
            line[len++] = c;
            putchar(c);
            break;
        }
    }
}