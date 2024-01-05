#include <stdio.h>
#include <unistd.h>
#include <errno.h>

extern int errno;

int main() {
   if(access("test.txt", F_OK) == -1) {
      printf("errno=%d\n", errno);
   }
}
