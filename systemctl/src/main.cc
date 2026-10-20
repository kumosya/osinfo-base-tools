#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "os/service_control.h"

namespace {
class Command {
    static const char* state(os::ServiceState value) {
        switch (value) {
            case os::ServiceState::Stopped: return "stopped";
            case os::ServiceState::Running: return "running";
            case os::ServiceState::Failed: return "failed";
            case os::ServiceState::Starting: return "starting";
        }
        return "unknown";
    }
    static void display(const os::ServiceStatus& value, bool detailed) {
        std::printf("%-23s %-8s %-9s %6llu %-8s\n", value.name, value.owner, state(value.state),
            static_cast<unsigned long long>(value.pid), value.enabled ? "enabled" : "disabled");
        if (detailed) std::printf("  executable: %s\n  desired: %s; builtin: %s; restarts: %u; last exit: %d\n",
            value.executable[0] ? value.executable : "boot module", value.desired ? "running" : "stopped",
            value.builtin ? "yes" : "no", value.restarts, value.exitCode);
    }
 public:
    static int run(int argc, char** argv) {
        if (argc > 1 && !strcmp(argv[1], "--help")) {
            std::puts("Usage: systemctl list | status [SERVICE] | start|stop|restart|enable|disable SERVICE"); return 0;
        }
        const char* action = argc > 1 ? argv[1] : "list";
        os::ServiceOperation operation{};
        if (!strcmp(action, "list")) operation = os::ServiceOperation::List;
        else if (!strcmp(action, "status")) operation = os::ServiceOperation::Status;
        else if (!strcmp(action, "start")) operation = os::ServiceOperation::Start;
        else if (!strcmp(action, "stop")) operation = os::ServiceOperation::Stop;
        else if (!strcmp(action, "restart")) operation = os::ServiceOperation::Restart;
        else if (!strcmp(action, "enable")) operation = os::ServiceOperation::Enable;
        else if (!strcmp(action, "disable")) operation = os::ServiceOperation::Disable;
        else { fputs("systemctl: unknown command; use --help\n", stderr); return 2; }
        if ((operation == os::ServiceOperation::List && argc > 2) || argc > 3 ||
            (operation != os::ServiceOperation::List && operation != os::ServiceOperation::Status && argc != 3) ||
            (argc == 3 && (!argv[2][0] || strlen(argv[2]) >= 24))) {
            fputs("systemctl: invalid arguments; use --help\n", stderr); return 2;
        }
        if (argc < 3) {
            std::puts("SERVICE                 OWNER    STATE        PID ENABLED");
            for (uint64_t i = 0; i < 64; ++i) {
                os::ServiceStatus status{};
                const int result = os::ServiceControl::call(os::ServiceOperation::List, nullptr, i, status);
                if (result == static_cast<int>(os::ErrorCode::NotFound) && i) return 0;
                if (result) { fprintf(stderr, "systemctl: service manager unavailable (%d)\n", result); return 1; }
                display(status, operation == os::ServiceOperation::Status);
            }
            fputs("systemctl: invalid service list length\n", stderr); return 1;
        }
        os::ServiceStatus status{};
        const int result = os::ServiceControl::call(operation, argv[2], 0, status);
        if (result) {
            const char* reason = result == static_cast<int>(os::ErrorCode::Busy) ? "active dependents prevent this operation" :
                result == static_cast<int>(os::ErrorCode::PermissionDenied) ? "permission denied or immutable bootstrap service" :
                result == static_cast<int>(os::ErrorCode::NotFound) ? "unknown service or unavailable owner" : "service operation failed";
            fprintf(stderr, "systemctl: %s: %s (%d)\n", argv[2], reason, result);
            if (status.blockedBy[0]) fprintf(stderr, "systemctl: required by %s\n", status.blockedBy);
            return 1;
        }
        display(status, true);
        return operation == os::ServiceOperation::Status && status.state != os::ServiceState::Running ? 3 : 0;
    }
};
}
int main(int argc, char** argv) { return Command::run(argc, argv); }
