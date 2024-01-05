#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    uid_t uid, euid;
    char *name;

    uid = getuid();
    euid = geteuid();
    name = getlogin();

    printf("Login Name=%s, UID=%d, EUID=%d\n", name, (int)uid, (int)euid);
}
