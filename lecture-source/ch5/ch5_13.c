#include <sys/types.h>
#include <utmp.h>
#include <stdio.h>

int main() {
    struct utmp *ut;

    printf("LoginName Line\n");
    printf("---------------\n");

    while ((ut=getutent()) != NULL) {
        if (ut->ut_type != USER_PROCESS)
            continue;
        printf("%s	  %s\n", ut->ut_user, ut->ut_line);
    }
}
