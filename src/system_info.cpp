#include "handler/system_info.h"

#include <cstdlib>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace handler {

namespace {

std::string environmentValue(const char* name) {
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string{};
}

} // namespace

SystemHealth inspectSystem() {
    SystemHealth health;
    health.computerName = environmentValue("COMPUTERNAME");
    health.userName = environmentValue("USERNAME");
    health.tempPath = environmentValue("TEMP");
    health.pathValue = environmentValue("PATH");
    health.pathAvailable = !health.pathValue.empty();
    health.tempAvailable = !health.tempPath.empty();
    return health;
}

void printSystemHealth(const SystemHealth& health) {
    std::cout << "Handler System Health\n";
    std::cout << "---------------------\n";
    std::cout << "Computer     : " << (health.computerName.empty() ? "<unknown>" : health.computerName) << '\n';
    std::cout << "User         : " << (health.userName.empty() ? "<unknown>" : health.userName) << '\n';
    std::cout << "TEMP         : " << (health.tempAvailable ? health.tempPath : "<missing>") << '\n';
    std::cout << "PATH         : " << (health.pathAvailable ? "available" : "missing") << '\n';
    std::cout << "Scope        : PC / System\n";
}

} // namespace handler
