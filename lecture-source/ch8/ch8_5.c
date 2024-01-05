#include <signal.h>
#include <stdio.h>

int main() {
    sigset_t st;

    sigemptyset(&st);

    sigaddset(&st, SIGINT);
    sigaddset(&st, SIGQUIT);

    if (sigismember(&st, SIGINT))
        printf("SIGINT is setting.\n");

    printf("** Bit Pattern: %lx\n",st.__val[0]);
}
