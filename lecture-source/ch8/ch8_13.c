#include <unistd.h>
#include <signal.h>
#include <stdio.h>

void sig_handler(int signo) {
    psignal(signo, "Received Signal:");
}

int main() {
    sigset_t set;

    signal(SIGALRM, sig_handler);

    sigfillset(&set);
    sigdelset(&set, SIGALRM);

    alarm(3);

    printf("Wait...\n");

    sigsuspend(&set);
}
