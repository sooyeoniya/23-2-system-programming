#include <stdlib.h>
#include <stdio.h>

int main() {
    char *val;

    val = getenv("TERM");
    if (val == NULL)
        printf("SHELL not defined\n");
    else
        printf("1. TERM = %s\n", val);

    putenv("TERM=vt100");

    val = getenv("TERM");
    printf("2. TERM = %s\n", val);
}
