#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PORTNUM 9004

int main() {
    char buf[256];
    struct sockaddr_in sin, cli;
    int sd, ns, clientlen = sizeof(cli);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    printf("** Create Socket\n");

    memset((char *)&sin, '\0', sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORTNUM);
    sin.sin_addr.s_addr = inet_addr("192.168.147.129");

    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin))) {
        perror("bind");
        exit(1);
    }
    printf("** Bind Socket\n");

    if (listen(sd, 5)) {
        perror("listen");
        exit(1);
    }
    printf("** Listen Socket\n");

    while (1) {
        if ((ns = accept(sd, (struct sockaddr *)&cli, &clientlen)) == -1) {
            perror("accept");
            exit(1);
        }
        printf("** Accept Client\n");

        switch (fork()) {
            case 0:
                printf("** Fork Client\n");
                close(sd);
                sprintf(buf, "%d", ns);
                execlp("./bit", "bit", buf, (char *)0);
                close(ns);
        }
        close(ns);
    }
}
