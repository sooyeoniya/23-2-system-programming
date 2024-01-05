#include <time.h>
#include <stdio.h>

int main() {
    tzset();

    printf("Timezone : %d\n", (int)timezone);
    printf("Daylight : %d\n", daylight);
    printf("TZname[0] : %s\n", tzname[0]);
    printf("TZname[1] : %s\n", tzname[1]);
}
