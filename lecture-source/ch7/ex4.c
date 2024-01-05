#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
    pid_t pid;

    switch (pid = fork()) {
        case -1 :
            printf("-1\n");
            break;
        case 1 :
            printf("1\n");
            break;
        case 0 :
            printf("0\n");
            execl("pwd", "pwd", (char *)NULL);
            break; 
        default :
            printf("default\n");
        break;
    }
}
