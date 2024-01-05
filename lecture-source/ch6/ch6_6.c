#include <unistd.h>
#include <stdio.h>

extern char **environ;

int main() {
    char **env;

    env = environ;
    while (*env) {
        printf("%s\n", *env);
        env++;
    }
}
