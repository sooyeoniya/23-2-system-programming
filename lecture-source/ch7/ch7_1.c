#include <stdlib.h>
#include <stdio.h>

int main() {
    int ret;
    ret = system("ps -ef | grep sshd > sshd.txt");
    printf("Return Value : %d\n", ret);
}
