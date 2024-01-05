#include <stdlib.h>
#include <stdio.h>

void cleanup1() {
    printf("Cleanup 1 is called.\n");
}

void cleanup2(int status, void *arg) {
    printf("Cleanup 2 is called: %ld.\n", (long) arg);
}

int main() {
    atexit(cleanup1);
    on_exit(cleanup2, (void *) 20);

    exit(0);
}
