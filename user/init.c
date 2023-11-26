#include "types.h"
#include "ulib.h"

int main()
{
    int pid;
    for (int i = 0; i < 5; ++i)
    {
        pid = fork();
        if (!pid)
        {
            printf("Hello world from child, pid = %d\n", getpid());
            break;
            // exec("hello");
        }
        else
        {
            printf("Hello world from parent, pid = %d\n", getpid());
        }
    }
    // while ((pid = wait()) != -1)
    // {
    //     printf("Child %d terminated!\n", pid);
    // }
    return 0;
}