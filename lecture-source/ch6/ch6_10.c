#include <stdlib.h>
#include <stdio.h>

int main() {
    char *val;

    val = getenv("TERM");
    if (val == NULL)
        printf("TERM not defined\n");
    else
        printf("1. TERM = %s\n", val);

    setenv("TERM","vt100", 0);
    val = getenv("TERM");
    printf("2. TERM = %s\n", val);

    setenv("TERM","vt100", 1);
    val = getenv("TERM");
    printf("3. TERM = %s\n", val);
}
