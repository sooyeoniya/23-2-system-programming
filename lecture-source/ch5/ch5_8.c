#include <shadow.h>
#include <stdio.h>

int main() {
    struct spwd *spw;

    spw = getspnam("user1");
    printf("Login Name : %s\n", spw->sp_namp);
    printf("Passwd : %s\n", spw->sp_pwdp);
    printf("Last Change : %ld\n", spw->sp_lstchg);
}
