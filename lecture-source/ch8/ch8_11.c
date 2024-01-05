#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void sig_handler(int signo) {
    char *s;

    s = strsignal(signo);
    printf("Received Signal : %s\n", s);
}

int main() {
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    sighold(SIGINT);

    pause();
}
