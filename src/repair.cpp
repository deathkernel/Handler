#include "handler/repair.h"

#include "handler/action_engine.h"
#include "handler/history.h"
#include "handler/policy.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace handler {

namespace {

bool validPackageName(const std::string& package) {
    if (package.empty() || package.size() > 128) return false;
    for (const unsigned char ch : package) {
        if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' ||
            ch == '[' || ch == ']')
            continue;
        return false;
    }
    return true;
}

std::filesystem::path repairStateRoot() {
    if (const char* p = std::getenv("LOCALAPPDATA"); p && *p)
        return std::filesystem::path(p) / "Handler";
    if (const char* p = std::getenv("USERPROFILE"); p && *p)
        return std::filesystem::path(p) / ".handler";
    return std::filesystem::current_path() / ".handler";
}

} // namespace

int repairPythonModule(const char* rawPackage) {
    const std::string package = rawPackage ? rawPackage : "";
    if (!validPackageName(package)) {
        std::cerr << "Repair blocked: invalid Python package name.\n";
        return 3;
    }

    std::cout << "Python repair requested for: " << package << "\n";
    std::cout << "Handler will run: python -m pip install " << package << "\n";

    const auto policy = evaluatePolicy(SafetyMode::Confirm, RiskLevel::High);
    if (policy.requiresConfirmation) {
        std::cout << policy.reason << " [y/N]: ";
        std::string answer;
        std::getline(std::cin, answer);
        if (answer != "y" && answer != "Y") {
            std::cout << "Repair cancelled.\n";
            return 0;
        }
    }

    CommandSpec install{
        "python-repair",
        "python",
        {"-m", "pip", "install", package, "--disable-pip-version-check"},
        RiskLevel::High,
        180000
    };

    const auto installResult = executeCommand(install);
    std::cout << installResult.output;

    History history(repairStateRoot() / "history.log");
    if (!installResult.started || installResult.exitCode != 0) {
        history.record("PYTHON_REPAIR_FAILED",
                       package + " | " + installResult.error);
        std::cerr << "Repair failed. No further action taken.\n";
        return installResult.started ? installResult.exitCode : 1;
    }

    CommandSpec verify{
        "python-repair-verify",
        "python",
        {"-m", "pip", "show", package},
        RiskLevel::Low,
        30000
    };

    const auto verifyResult = executeCommand(verify);
    if (!verifyResult.started || verifyResult.exitCode != 0) {
        history.record("PYTHON_REPAIR_UNVERIFIED", package);
        std::cerr << "Package installation completed but verification failed.\n";
        return 1;
    }

    std::cout << "Repair verified: " << package << " is installed.\n";
    history.record("PYTHON_REPAIR_SUCCESS", package);
    return 0;
}

} // namespace handler
