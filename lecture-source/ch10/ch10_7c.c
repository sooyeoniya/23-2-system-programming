#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
    int pd, n;
    char inmsg[80];

    if ((pd = open("./HAN-FIFO", O_RDONLY)) == -1) {
        perror("open");
        exit(1);
    }

    printf("Client =====\n");
    write(1, "From Server :", 13);

    while ((n=read(pd, inmsg, 80)) > 0)
        write(1, inmsg, n);

    if (n == -1) {
        perror("read");
        exit(1);
    }

    write(1, "\n", 1);
    close(pd);
}
