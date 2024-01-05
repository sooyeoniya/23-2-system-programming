#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

extern int errno;

int  main() {
    FILE *fp;

    if((fp=fopen("test.txt", "r")) == NULL) {
       printf("errno=%d\n", errno);
       exit(1);
    }
    fclose(fp);
}
