#include <cstdio>
#include <cstring>
#include "../user-prompt.h"
#include "os/task.h"
#include "../processes.h"
int main(int, char**) {
    puts("USER       PID     HOME");
    std::vector<coreutils::ProcessRecord> records;
    if (!coreutils::processes(records)) { perror("/proc"); return 1; }
    for (const auto& task : records) {
        const char* name = strrchr(task.name, '/'); name = name ? name + 1 : task.name;
        if (task.state == 2 || strcmp(name, "sh")) continue;
        os::UserProfile profile{};
        if (coreutils::account(task.uid, profile)) printf("%-10s %-7llu %s\n", profile.name,
            static_cast<unsigned long long>(task.pid), profile.home);
    }
    return 0;
}
