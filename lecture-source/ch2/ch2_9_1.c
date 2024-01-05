#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

int main() {
    DIR *dp;
    struct dirent *dent;
    long loc, loc3;
    int pos;

    dp = opendir(".");

    printf("Start Position : %ld\n", telldir(dp));
    pos = 1;
    while ((dent = readdir(dp))) {
        printf("Read : %s ", dent->d_name);
        loc = telldir(dp);
        printf("Cur Position : %ld\n", loc);
        pos++;
        if (pos == 3) loc3 = loc;
    }

    printf("** Directory Position Rewind **\n");
    rewinddir(dp);
    printf("Cur Position : %ld\n", telldir(dp));

    printf("** Move Directory Pointer **\n");
    seekdir(dp, loc3);
    printf("Cur Position : %ld\n", telldir(dp));

    dent = readdir(dp);
    printf("Read %s\n", dent->d_name);

    closedir(dp);
}
