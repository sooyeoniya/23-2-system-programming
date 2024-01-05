#include <stdio.h>
#include <stdlib.h>

int main() {
    char *ptr, *new;
    ptr = malloc(sizeof(char) * 100);
/*
    ptr = calloc(10, 20);
*/
    new = realloc(ptr, 100);

    printf("addr: %p\n", ptr);
}
