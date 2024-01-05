#include <grp.h>
#include <stdio.h>

int main() {
    struct group *grp;
    int n;

    grp = getgrnam("adm");
    printf("Group Name : %s\n", grp->gr_name);
    printf("GID : %d\n", (int)grp->gr_gid);

    n = 0;
    printf("Members : ");
    while (grp->gr_mem[n] != NULL)
        printf("%s ", grp->gr_mem[n++]);
    printf("\n");
}
