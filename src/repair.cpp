#include "handler/repair.h"

#include "handler/action_engine.h"
#include "handler/history.h"
#include "handler/policy.h"
#include "handler/project_context.h"
#include "handler/transaction.h"
#include "handler/artifact_backup.h"
#include "handler/verification.h"
#include "handler/state_paths.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

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
    return handlerStateRoot();
}

std::filesystem::path pythonExecutableForCurrentContext() {
    if (const char* v = std::getenv("VIRTUAL_ENV"); v && *v) {
        const auto root = std::filesystem::path(v);
#ifdef _WIN32
        const auto candidate = root / "Scripts" / "python.exe";
#else
        const auto candidate = root / "bin" / "python";
#endif
        std::error_code ec;
        if (std::filesystem::is_regular_file(candidate, ec))
            return candidate;
    }

    const auto project = detectProjectContext(std::filesystem::current_path());
    if (!project.root.empty() && project.type == "Python") {
        for (const auto& name : {".venv", "venv"}) {
#ifdef _WIN32
            const auto candidate = project.root / name / "Scripts" / "python.exe";
#else
            const auto candidate = project.root / name / "bin" / "python";
#endif
            std::error_code ec;
            if (std::filesystem::is_regular_file(candidate, ec))
                return candidate;
        }
    }

    return {};
}

std::filesystem::path nodeWorkingDirectory() {
    const auto project = detectProjectContext(std::filesystem::current_path());
    if (!project.root.empty() && project.type == "Node.js")
        return project.root;
    return {};
}

} // namespace

int repairPythonModule(const char* rawPackage) {
    const std::string package = rawPackage ? rawPackage : "";
    if (!validPackageName(package)) {
        std::cerr << "Repair blocked: invalid Python package name.\n";
        return 3;
    }

    const auto pythonPath = pythonExecutableForCurrentContext();
    const std::string target = pythonPath.empty() ? "PATH Python" : pythonPath.string();
    std::cout << "Python repair requested for: " << package << "\n"
              << "Target interpreter: " << target << "\n";

    CommandSpec precheck{"python-repair-precheck", "python",
                         {"-m", "pip", "show", package}, RiskLevel::Low, 30000};
    precheck.executablePath = pythonPath;
    const auto before = executeCommand(precheck);
    const bool wasInstalled = before.started && before.exitCode == 0;

    const auto policy = evaluatePolicy(SafetyMode::Confirm, RiskLevel::High);
    if (policy.requiresConfirmation) {
        std::cout << policy.reason << " [y/N]: ";
        std::string answer;
        std::getline(std::cin, answer);
        if (answer != "y" && answer != "Y") {
            std::cout << "Repair cancelled.\n";
            return kRepairCancelled;
        }
    }

    Transaction tx(SafetyMode::Confirm);
    const auto artifactRoot = repairStateRoot() / "transactions" / "artifacts";
    std::vector<ArtifactBackup> backups;
    for (const auto& file : {std::filesystem::path("requirements.txt"),
                             std::filesystem::path("pyproject.toml")}) {
        const auto backup = backupArtifact(file, artifactRoot / "python");
        if (backup) backups.push_back(*backup);
    }
    const auto result = tx.runApproved(
        RiskLevel::High,
        [&] {
            CommandSpec install{"python-repair", "python",
                {"-m", "pip", "install", package, "--disable-pip-version-check"},
                RiskLevel::High, 180000};
            install.executablePath = pythonPath;
            const auto r = executeCommand(install);
            std::cout << r.output;
            return r.started && r.exitCode == 0;
        },
        [&] {
            CommandSpec verify{"python-repair-verify", "python",
                {"-m", "pip", "show", package}, RiskLevel::Low, 30000};
            verify.executablePath = pythonPath;
            const auto r = executeCommand(verify);
            if (!r.started || r.exitCode != 0)
                return VerificationResult{false, "pip show", r.error};
            CommandSpec consistency{"python-repair-consistency", "python",
                {"-m", "pip", "check"}, RiskLevel::Low, 30000};
            consistency.executablePath = pythonPath;
            const auto check = executeCommand(consistency);
            return VerificationResult{check.started && check.exitCode == 0,
                                      "pip show + pip check",
                                      check.exitCode == 0 ? "" : check.error};
        },
        [&] {
            for (const auto& backup : backups) (void)restoreArtifact(backup);
            if (wasInstalled) return;
            CommandSpec rollback{"python-repair-rollback", "python",
                {"-m", "pip", "uninstall", "-y", package,
                 "--disable-pip-version-check"}, RiskLevel::High, 120000};
            rollback.executablePath = pythonPath;
            (void)executeCommand(rollback);
        });

    History history(repairStateRoot() / "history.log");
    if (!result.committed) {
        history.record("PYTHON_REPAIR_FAILED",
                       package + " | " + result.details +
                       " | snapshot=" + result.snapshotId);
        std::cerr << "Python repair did not commit: " << result.details << "\n";
        return 1;
    }

    std::cout << "Repair verified and committed: " << package << "\n";
    history.record("PYTHON_REPAIR_SUCCESS",
                   package + " | " + target + " | snapshot=" + result.snapshotId);
    return 0;
}

int repairNodeModule(const char* rawPackage) {
    const std::string package = rawPackage ? rawPackage : "";
    if (!validPackageName(package)) {
        std::cerr << "Repair blocked: invalid Node package name.\n";
        return 3;
    }

    const auto projectRoot = nodeWorkingDirectory();
    if (projectRoot.empty()) {
        std::cerr << "Node repair blocked: no package.json project detected.\n";
        return 3;
    }

    std::cout << "Node repair requested for: " << package << "\n"
              << "Target project: " << projectRoot << "\n";

    CommandSpec precheck{"node-repair-precheck", "npm",
                         {"ls", package, "--depth=0"}, RiskLevel::Low, 30000};
    precheck.workingDirectory = projectRoot;
    const auto before = executeCommand(precheck);
    const bool wasInstalled = before.started && before.exitCode == 0;

    const auto policy = evaluatePolicy(SafetyMode::Confirm, RiskLevel::High);
    if (policy.requiresConfirmation) {
        std::cout << policy.reason << " [y/N]: ";
        std::string answer;
        std::getline(std::cin, answer);
        if (answer != "y" && answer != "Y") {
            std::cout << "Repair cancelled.\n";
            return kRepairCancelled;
        }
    }

    Transaction tx(SafetyMode::Confirm);
    const auto artifactRoot = repairStateRoot() / "transactions" / "artifacts";
    std::vector<ArtifactBackup> backups;
    for (const auto& file : {projectRoot / "package.json", projectRoot / "package-lock.json"}) {
        const auto backup = backupArtifact(file, artifactRoot / "node");
        if (backup) backups.push_back(*backup);
    }
    const auto result = tx.runApproved(
        RiskLevel::High,
        [&] {
            CommandSpec install{"node-repair", "npm",
                {"install", package, "--no-audit", "--no-fund"},
                RiskLevel::High, 180000};
            install.workingDirectory = projectRoot;
            const auto r = executeCommand(install);
            std::cout << r.output;
            return r.started && r.exitCode == 0;
        },
        [&] {
            CommandSpec verify{"node-repair-verify", "npm",
                {"ls", package, "--depth=0"}, RiskLevel::Low, 30000};
            verify.workingDirectory = projectRoot;
            const auto r = executeCommand(verify);
            return VerificationResult{r.started && r.exitCode == 0,
                                      "npm ls", r.error};
        },
        [&] {
            for (const auto& backup : backups) (void)restoreArtifact(backup);
            if (wasInstalled) return;
            CommandSpec rollback{"node-repair-rollback", "npm",
                {"uninstall", package, "--no-audit", "--no-fund"},
                RiskLevel::High, 120000};
            rollback.workingDirectory = projectRoot;
            (void)executeCommand(rollback);
        });

    History history(repairStateRoot() / "history.log");
    if (!result.committed) {
        history.record("NODE_REPAIR_FAILED",
                       package + " | " + result.details +
                       " | snapshot=" + result.snapshotId);
        std::cerr << "Node repair did not commit: " << result.details << "\n";
        return 1;
    }

    std::cout << "Repair verified and committed: " << package << "\n";
    history.record("NODE_REPAIR_SUCCESS",
                   package + " | " + projectRoot.string() +
                   " | snapshot=" + result.snapshotId);
    return 0;
}

} // namespace handler
