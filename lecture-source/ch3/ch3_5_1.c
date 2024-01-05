#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

int main() {
    struct stat statbuf;

    stat("/usr/bin/passwd", &statbuf);
    printf("Mode = %o\n", (unsigned int)statbuf.st_mode);

    if ((statbuf.st_mode & S_ISUID) != 0)
        printf("/usr/bin/passwd: setuid file\n");
}
