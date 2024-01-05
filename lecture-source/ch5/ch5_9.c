#include <shadow.h>
#include <stdio.h>

int main() {
    struct spwd *spw;
    int n;

    for (n = 0; n < 3; n++) {
        spw = getspent();
        printf("LoginName: %s, Passwd: %s\n", spw->sp_namp, spw->sp_pwdp);
    }
}
