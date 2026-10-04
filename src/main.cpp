#include "handler/system_info.h"
#include "handler/temp_cleaner.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

namespace {

constexpr int kVersionMajor = 0;
constexpr int kVersionMinor = 1;
constexpr int kVersionPatch = 0;
constexpr auto kMaintenanceInterval = std::chrono::hours(2);

void printUsage() {
    std::cout
        << "Handler - Developer Environment Guardian\n\n"
        << "Usage:\n"
        << "  handler health             Inspect PC/system environment\n"
        << "  handler self-check         Run Handler's own basic checks\n"
        << "  handler temp-cleanup       Safely clean %TEMP%\n"
        << "  handler maintenance        Run TEMP cleanup every 2 hours\n"
        << "  handler version            Show Handler version\n"
        << "  handler help               Show this help\n";
}

bool runSelfCheck() {
    const auto health = handler::inspectSystem();
    bool ok = true;

    std::cout << "Handler Self-Check\n";
    std::cout << "------------------\n";

    if (health.tempAvailable) {
        std::cout << "[OK] TEMP environment variable\n";
    } else {
        std::cout << "[FAIL] TEMP environment variable\n";
        ok = false;
    }

    if (health.pathAvailable) {
        std::cout << "[OK] PATH environment variable\n";
    } else {
        std::cout << "[WARN] PATH environment variable is missing\n";
    }

    std::cout << (ok ? "\nSelf-check passed.\n" : "\nSelf-check found a required problem.\n");
    return ok;
}

bool runTempCleanup() {
    const auto health = handler::inspectSystem();
    if (!health.tempAvailable) {
        std::cerr << "Handler: TEMP is not available. Cleanup skipped.\n";
        return false;
    }

    std::cout << "Cleaning: " << health.tempPath << '\n';
    const auto result = handler::cleanTempDirectory(
        std::filesystem::path(health.tempPath));

    handler::printCleanupResult(result);
    return true;
}

void runMaintenanceLoop() {
    std::cout << "Handler maintenance mode started.\n";
    std::cout << "TEMP cleanup interval: 2 hours.\n";
    std::cout << "Press Ctrl+C to stop.\n\n";

    while (true) {
        const auto now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        std::cout << "Maintenance check: " << std::ctime(&now);
        runTempCleanup();
        std::cout << "Next cleanup in 2 hours.\n\n";
        std::this_thread::sleep_for(kMaintenanceInterval);
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 0;
    }

    const std::string command = argv[1];

    if (command == "health") {
        handler::printSystemHealth(handler::inspectSystem());
        return 0;
    }

    if (command == "self-check") {
        return runSelfCheck() ? 0 : 1;
    }

    if (command == "temp-cleanup") {
        return runTempCleanup() ? 0 : 1;
    }

    if (command == "maintenance") {
        runMaintenanceLoop();
        return 0;
    }

    if (command == "version") {
        std::cout << "Handler " << kVersionMajor << '.'
                  << kVersionMinor << '.' << kVersionPatch << '\n';
        return 0;
    }

    if (command == "help" || command == "--help" || command == "-h") {
        printUsage();
        return 0;
    }

    std::cerr << "Unknown command: " << command << "\n\n";
    printUsage();
    return 2;
}
