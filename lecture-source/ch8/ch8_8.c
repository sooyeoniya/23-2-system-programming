#include <sys/ucontext.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

void sig_handler(int signo, siginfo_t *sf, ucontext_t *uc) {
    psiginfo(sf, "Received Signal:");
    printf("si_code : %d\n", sf->si_code);
}

int main() {
    struct sigaction act;

    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = (void (*)(int, siginfo_t *, void *))sig_handler;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGUSR1, &act, (struct sigaction *)NULL) < 0) {
        perror("sigaction");
        exit(1);
    }
    pause();
}
