#include "../processes.h"
#include <pwd.h>
#include <ctime>

int main(int argc, char** argv) {
    bool all = false, terminals = false, own = false, full = false, explicitSelection = false;
    const char* selectedTty = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-' || !argv[i][1]) { fputs("ps: expected an option\n", stderr); return 2; }
        for (const char* p = argv[i] + 1; *p; ++p) {
            if (*p == 'e' || *p == 'A') all = explicitSelection = true;
            else if (*p == 'a') terminals = explicitSelection = true;
            else if (*p == 'x') own = explicitSelection = true;
            else if (*p == 'f') full = true;
            else if (*p == 't') {
                if (p[1]) selectedTty = p + 1;
                else if (++i < argc) selectedTty = argv[i];
                else { fputs("ps: -t requires a TTY\n", stderr); return 2; }
                if (!strncmp(selectedTty, "/dev/", 5)) selectedTty += 5;
                explicitSelection = true; break;
            } else { fputs("usage: ps [-aAxef] [-t TTY]\n", stderr); return 2; }
        }
    }
    coreutils::ProcessRecord self{};
    if (!coreutils::processStatus("/proc/self", self)) { perror("/proc/self/status"); return 1; }
    std::vector<coreutils::ProcessRecord> records;
    if (!coreutils::processes(records)) { perror("/proc"); return 1; }
    std::puts(full ? "UID        PID   PPID  C STIME    TTY       TIME CMD" : "   PID TTY       TIME CMD");
    timespec now{}; clock_gettime(CLOCK_MONOTONIC, &now);
    const uint64_t uptime = static_cast<uint64_t>(now.tv_sec) * 1000000000 + now.tv_nsec;
    for (const auto& record : records) {
        char tty[32]; record.tty(tty, sizeof(tty));
        bool select = all || (terminals && record.ttyKind && record.pid != record.sid) ||
            (own && record.euid == self.euid) || (selectedTty && !strcmp(selectedTty, tty));
        if (!explicitSelection) select = record.euid == self.euid && self.ttyKind &&
            record.ttyKind == self.ttyKind && record.ttyIndex == self.ttyIndex;
        if (!select) continue;
        const uint64_t seconds = record.cpu / 1000000000;
        char cpuTime[40]; std::snprintf(cpuTime, sizeof(cpuTime), "%02llu:%02llu:%02llu",
            static_cast<unsigned long long>(seconds / 3600), static_cast<unsigned long long>(seconds / 60 % 60), static_cast<unsigned long long>(seconds % 60));
        if (full) {
            char uid[32]; passwd* account = getpwuid(record.euid);
            if (account) std::snprintf(uid, sizeof(uid), "%s", account->pw_name);
            else std::snprintf(uid, sizeof(uid), "%llu", static_cast<unsigned long long>(record.euid));
            const auto elapsed = uptime > record.start ? uptime - record.start : 0;
            const auto percent = elapsed ? record.cpu * 100 / elapsed : 0;
            const time_t current = time(nullptr);
            const time_t started = current - static_cast<time_t>(uptime / 1000000000) + static_cast<time_t>(record.start / 1000000000);
            tm broken{}; char stamp[24] = "?";
            if (localtime_r(&started, &broken)) std::strftime(stamp, sizeof(stamp), "%H:%M:%S", &broken);
            std::printf("%-8s %5llu %6llu %2llu %s %-8s %s %s\n", uid, static_cast<unsigned long long>(record.pid),
                static_cast<unsigned long long>(record.ppid), static_cast<unsigned long long>(percent), stamp, tty, cpuTime, record.command);
        } else std::printf("%6llu %-8s %s %s\n", static_cast<unsigned long long>(record.pid), tty, cpuTime, record.name);
    }
    return 0;
}
