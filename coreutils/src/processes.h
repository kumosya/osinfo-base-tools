#pragma once
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <dirent.h>
#include <vector>

namespace coreutils {
struct ProcessRecord {
    uint64_t pid = 0, ppid = 0, uid = 0, euid = 0, gid = 0, pgid = 0, sid = 0, start = 0, cpu = 0;
    unsigned state = 0, ttyKind = 0, ttyIndex = 0, serviceRole = 0;
    char name[64]{}, command[512]{};
    void tty(char* output, size_t size) const {
        if (ttyKind == 1) snprintf(output, size, "tty%u", ttyIndex + 1);
        else if (ttyKind == 2) snprintf(output, size, "pts/%u", ttyIndex);
        else snprintf(output, size, "?");
    }
};
inline bool processStatus(const char* directory, ProcessRecord& record) {
    char path[128], line[512]; snprintf(path, sizeof(path), "%s/status", directory);
    FILE* file = fopen(path, "r"); if (!file) return false;
    while (fgets(line, sizeof(line), file)) {
        char* colon = strchr(line, ':'); if (!colon) continue;
        *colon++ = 0; while (*colon == ' ' || *colon == '\t') ++colon;
        const uint64_t number = strtoull(colon, nullptr, 10);
        if (!strcmp(line, "Name")) { colon[strcspn(colon, "\r\n")] = 0; snprintf(record.name, sizeof(record.name), "%s", colon); }
        else if (!strcmp(line, "Pid")) record.pid = number;
        else if (!strcmp(line, "PPid")) record.ppid = number;
        else if (!strcmp(line, "Uid")) record.uid = number;
        else if (!strcmp(line, "Euid")) record.euid = number;
        else if (!strcmp(line, "Gid")) record.gid = number;
        else if (!strcmp(line, "Pgid")) record.pgid = number;
        else if (!strcmp(line, "Sid")) record.sid = number;
        else if (!strcmp(line, "State")) record.state = number;
        else if (!strcmp(line, "TtyKind")) record.ttyKind = number;
        else if (!strcmp(line, "TtyIndex")) record.ttyIndex = number;
        else if (!strcmp(line, "StartNs")) record.start = number;
        else if (!strcmp(line, "CpuNs")) record.cpu = number;
        else if (!strcmp(line, "ServiceRole")) record.serviceRole = number;
    }
    const bool valid = !ferror(file) && record.pid; fclose(file); if (!valid) return false;
    snprintf(path, sizeof(path), "%s/cmdline", directory); file = fopen(path, "r");
    if (file) {
        const size_t count = fread(record.command, 1, sizeof(record.command) - 1, file); fclose(file);
        for (size_t i = 0; i < count; ++i) { const unsigned char c = record.command[i]; record.command[i] = !c ? ' ' : c < 32 || c == 127 ? '?' : c; }
        size_t end = count; while (end && record.command[end - 1] == ' ') --end; record.command[end] = 0;
    }
    if (!record.command[0]) snprintf(record.command, sizeof(record.command), "%s", record.name);
    return true;
}
inline bool processes(std::vector<ProcessRecord>& records) {
    DIR* directory = opendir("/proc"); if (!directory) return false;
    errno = 0;
    while (dirent* entry = readdir(directory)) {
        if (entry->d_name[0] < '1' || entry->d_name[0] > '9') continue;
        char* end = nullptr; const auto pid = strtoull(entry->d_name, &end, 10); if (!pid || !end || *end) continue;
        char path[128]; snprintf(path, sizeof(path), "/proc/%llu", static_cast<unsigned long long>(pid));
        ProcessRecord record{}; if (processStatus(path, record)) records.push_back(record);
        errno = 0;
    }
    int error = errno; if (closedir(directory)) error = errno;
    if (error) { errno = error; return false; } return true;
}
}
