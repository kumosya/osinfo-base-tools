#include <csignal>
#include <cstdio>
#include <unistd.h>
int main(int, char**) {
    if (kill(getppid(), SIGHUP)) { perror("logout"); return 1; }
    return 0;
}
