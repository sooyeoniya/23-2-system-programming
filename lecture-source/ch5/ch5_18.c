#include <time.h>
#include <stdio.h>

int main() {
    struct tm tm;
    time_t timep;

    time(&timep);
    printf("Current Time(sec) : %d\n", (int)timep);

    tm.tm_year = 121;
    tm.tm_mon = 4;
    tm.tm_mday = 1;
    tm.tm_hour = 23;
    tm.tm_min = 59;
    tm.tm_sec = 0;

    timep = mktime(&tm);
    printf("2021/05/01 23:59:00 Time(sec) : %d\n", (int)timep);
}
