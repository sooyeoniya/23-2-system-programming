#include <pwd.h>
#include <stdio.h>

int main() {
    struct passwd *pw;
    int n;

    for (n = 0; n < 3; n++) {
        pw = getpwent();
        printf("UID: %d, LoginName: %s\n", (int)pw->pw_uid, pw->pw_name);
    }
}
