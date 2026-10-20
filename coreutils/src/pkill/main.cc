#include <csignal>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "os/task.h"
#include "../signals.h"
#include "../processes.h"

int main(int argc, char** argv) {
    bool exact = strstr(argv[0], "killall") != nullptr;
    int signal = SIGTERM, index = 1;
    if (index < argc && !strcmp(argv[index], "-x")) { exact = true; ++index; }
    if (index < argc && argv[index][0] == '-') signal = coreutils::parseSignal(argv[index++] + 1);
    if (signal < 0 || index == argc) { fputs("usage: pkill [-x] [-SIGNAL] NAME ...\n", stderr); return 2; }
    unsigned matched = 0;
    int result = 0;
    std::vector<coreutils::ProcessRecord> targets;
    if (!coreutils::processes(targets)) { perror("/proc"); return 1; }
    for (const auto& task : targets) {
        if (task.pid == static_cast<uint64_t>(getpid()) || task.state == 2) continue;
        const char* name = strrchr(task.name, '/'); name = name ? name + 1 : task.name;
        bool match = false;
        for (int n = index; n < argc; ++n)
            match = match || (exact ? !strcmp(name, argv[n]) : strstr(name, argv[n]) != nullptr);
        if (!match) continue;
        ++matched;
        if (kill(task.pid, signal)) { perror(name); result = 1; }
    }
    return matched ? result : 1;
}
