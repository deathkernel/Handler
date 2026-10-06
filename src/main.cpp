#include "handler/tool_updates.h"
#include "handler/risky_command.h"
#include "handler/safe_mode.h"
#include "handler/deep_cleanup.h"
#include "handler/uninstall.h"
#include "handler/environment_health.h"
#include "handler/path_guardian.h"
#include "handler/environment_guardian.h"
#include "handler/resource_guardian.h"
#include "handler/toolchain_doctor.h"
#include "handler/action_engine.h"
#include "handler/decision_engine.h"
#include "handler/recovery.h"
#include "handler/component_discovery.h"
#include "handler/dependency_graph.h"
#include "handler/dependency_manager.h"
#include "handler/environment_state.h"
#include "handler/error_detection.h"
#include "handler/history.h"
#include "handler/maintenance.h"
#include "handler/module.h"
#include "handler/project_context.h"
#include "handler/policy.h"
#include "handler/recovery_journal.h"
#include "handler/snapshot.h"
#include "handler/transaction.h"
#include "handler/verification.h"
#include "handler/router.h"
#include "handler/state_store.h"
#include "handler/system_info.h"
#include "handler/temp_cleaner.h"
#include "handler/repair.h"
#include "handler/auto_recovery.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr const char* kVersion = "0.7.0";

std::filesystem::path stateRoot() {
    if (const char* p = std::getenv("LOCALAPPDATA"); p && *p)
        return std::filesystem::path(p) / "Handler";
    if (const char* p = std::getenv("USERPROFILE"); p && *p)
        return std::filesystem::path(p) / ".handler";
    return std::filesystem::current_path() / ".handler";
}

handler::History makeHistory() {
    return handler::History(stateRoot() / "history.log");
}

void printDecisions(const std::vector<handler::Decision>& decisions) {
    for (const auto& d : decisions)
        std::cout << d.action << " | risk=" << static_cast<int>(d.risk)
                  << " | " << d.reason << "\n";
}

int runUpdates() {
    for (const auto& u : handler::inspectToolUpdates(
             {"python", "node", "git", "cmake", "dotnet"})) {
        std::cout << u.tool << ": "
                  << (u.currentPath.empty() ? "missing" : u.currentPath)
                  << " | " << u.updateHint << "\n";
    }
    return 0;
}

std::string joinArguments(int argc, char* argv[], int start) {
    std::string result;
    for (int i = start; i < argc; ++i) {
        if (!result.empty()) result += ' ';
        result += argv[i];
    }
    return result;
}

int runRisk(const std::string& line) {
    const auto r = handler::inspectCommandRisk(line);
    const char* label = r.risk == handler::CommandRisk::Safe ? "SAFE"
                      : r.risk == handler::CommandRisk::Review ? "REVIEW"
                      : "BLOCKED";
    std::cout << "Risk: " << label << " | " << r.reason << "\n";
    return r.risk == handler::CommandRisk::Blocked ? 3 : 0;
}

int runSafeMode() {
    const auto c = handler::createSafeModeContext(std::filesystem::current_path());
    std::cout << "Safe mode: " << (c.enabled ? "ready" : "failed")
              << "\nSandbox: " << c.root << "\n";
    return c.enabled ? 0 : 1;
}

handler::ProjectContext currentProject();

int runDoctor() {
    std::cout << "Handler Doctor\n==============\n";
    const auto health = handler::inspectSystem();
    handler::printSystemHealth(health);

    std::cout << "\nToolchain:\n";
    for (const auto& tool : handler::inspectToolchain(
             {"python", "node", "git", "cmake", "dotnet", "java", "go", "cargo"})) {
        std::cout << "  [" << (tool.available ? "OK" : "MISSING") << "] "
                  << tool.tool << ": " << tool.details << "\n";
    }

    std::cout << "\nPorts:\n";
    for (const auto& port : handler::inspectPorts({3000, 5000, 8000, 8080})) {
        std::cout << "  [" << (port.available ? "FREE" : "BUSY") << "] "
                  << port.port << ": " << port.details << "\n";
    }

    const auto context = currentProject();
    std::cout << "\nProject: "
              << (context.root.empty() ? "not detected" : context.root.string())
              << "\n";

    makeHistory().record("DOCTOR", "environment/toolchain/resource/project checks completed");
    return 0;
}

int runProtection() {
    handler::printEnvironmentHealth(handler::inspectEnvironmentHealth());
    for (const auto& f : handler::inspectPathEntries())
        std::cout << "[" << (f.exists ? "OK" : "WARN") << "] PATH "
                  << f.entry << ": " << f.details << "\n";
    for (const auto& v : handler::inspectEnvironmentVariables(
             {"TEMP", "PATH", "USERPROFILE", "LOCALAPPDATA"}))
        std::cout << "[" << (v.present ? "OK" : "WARN") << "] ENV "
                  << v.name << ": " << v.details << "\n";
    for (const auto& p : handler::inspectPorts({3000, 5000, 8000, 8080}))
        std::cout << "[" << (p.available ? "OK" : "BUSY") << "] PORT "
                  << p.port << ": " << p.details << "\n";
    for (const auto& t : handler::inspectToolchain(
             {"python", "git", "node", "cmake", "dotnet"}))
        std::cout << "[" << (t.available ? "OK" : "WARN") << "] TOOL "
                  << t.tool << ": " << t.details << "\n";
    makeHistory().record("PROTECTION_CHECK",
        "environment/path/variables/ports/toolchain inspected");
    return 0;
}

int runDecide(const std::string& text) {
    const auto errors = handler::detectErrors(text);
    const auto decisions = handler::decideRepairs(errors);
    printDecisions(decisions);
    makeHistory().record("DECISION",
        "actions=" + std::to_string(decisions.size()));
    return decisions.empty() ? 0 : 1;
}

int runCommand(const std::string& executable,
               const std::vector<std::string>& args) {
    if (!handler::isAllowedExecutable(executable)) {
        std::cerr << "Command blocked: executable is not allowlisted.\n";
        return 3;
    }

    handler::CommandSpec spec{"cli-command", executable, args,
                              handler::classifyCommandRisk(executable, args)};
    std::cout << handler::buildCommandLine(spec) << "\n";

    const auto policy =
        handler::evaluatePolicy(handler::SafetyMode::Confirm, spec.risk);

    if (policy.requiresConfirmation) {
        std::cout << policy.reason << " [y/N]: ";
        std::string answer;
        std::getline(std::cin, answer);
        if (answer != "y" && answer != "Y") {
            std::cout << "Command cancelled.\n";
            return 0;
        }
    }
    if (!policy.allowed && !policy.requiresConfirmation) {
        std::cout << "Command blocked: " << policy.reason << "\n";
        return 3;
    }

    const auto result = handler::executeCommand(spec);
    std::cout << result.output;
    return result.started ? result.exitCode : 1;
}

void printUsage() {
    std::cout
        << "Handler - Developer Environment Guardian\n\n"
        << "Usage:\n"
        << "  handler health                 Inspect PC/system environment\n"
        << "  handler self-check             Run Handler's own checks\n"
        << "  handler temp-cleanup           Safely clean %TEMP%\n"
        << "  handler temp-cleanup --dry-run Preview TEMP cleanup\n"
        << "  handler maintenance            Run maintenance every 2 hours\n"
        << "  handler state                  Capture current environment state\n"
        << "  handler detect-error <text>    Detect known error patterns\n"
        << "  handler discover [tools...]    Discover tools on PATH\n"
        << "  handler project                Detect project context\n"
        << "  handler deps                   Inspect project dependencies\n"
        << "  handler graph                  Show dependency edges\n"
        << "  handler decide <error text>    Generate repair decisions\n"
        << "  handler command <tool> [...]   Execute an allowlisted command\n"
        << "  handler protect                Run protection diagnostics\n"
        << "  handler doctor                 Run complete deterministic diagnostics\n"
        << "  handler updates                Inspect tool updates\n"
        << "  handler risk <command>         Inspect risky command patterns\n"
        << "  handler repair python-module <package> Repair a Python module safely\n"
        << "  handler repair node-module <package>   Repair a Node module in the detected project\n"
        << "  handler recover <error text>       Detect and propose a guarded repair\n"
        << "  handler safe-mode              Prepare isolated sandbox context\n"
        << "  handler modules                Show registered modules\n"
        << "  handler status                 Show saved environment status\n"
        << "  handler history                Show recent Handler events\n"
        << "  handler snapshots              List recovery snapshots\n"
        << "  handler rollback <id>          Restore Handler baseline from snapshot\n"
        << "  handler version                Show Handler version\n"
        << "  handler help                   Show this help\n";
}

handler::StateStore makeStateStore() {
    return handler::StateStore(stateRoot() / "state");
}

int runHealth() {
    const auto health = handler::inspectSystem();
    handler::printSystemHealth(health);
    const auto state = handler::captureEnvironmentState();
    if (!makeStateStore().saveCurrent(state))
        std::cerr << "[WARN] Could not persist environment state.\n";
    makeHistory().record("HEALTH_CHECK", "system observation completed");
    return 0;
}

int runSelfCheck() {
    const auto health = handler::inspectSystem();
    bool ok = true;
    std::cout << "Handler Self-Check\n------------------\n";

    if (health.tempAvailable)
        std::cout << "[OK] TEMP environment variable\n";
    else {
        std::cout << "[FAIL] TEMP environment variable\n";
        ok = false;
    }

    if (health.pathAvailable)
        std::cout << "[OK] PATH environment variable\n";
    else
        std::cout << "[WARN] PATH environment variable is missing\n";

    if (makeStateStore().saveCurrent(handler::captureEnvironmentState()))
        std::cout << "[OK] Environment state store\n";
    else {
        std::cout << "[FAIL] Environment state store\n";
        ok = false;
    }

    makeHistory().record("SELF_CHECK", ok ? "passed" : "failed");
    std::cout << (ok ? "\nSelf-check passed.\n"
                     : "\nSelf-check found a required problem.\n");
    return ok ? 0 : 1;
}

int runTempCleanup(bool dryRun = false) {
    const auto health = handler::inspectSystem();
    if (!health.tempAvailable) {
        std::cerr << "Handler: TEMP is not available. Cleanup skipped.\n";
        makeHistory().record("TEMP_CLEANUP_SKIPPED", "TEMP unavailable");
        return 1;
    }

    if (!dryRun) {
        const auto policy = handler::evaluatePolicy(
            handler::SafetyMode::Confirm, handler::RiskLevel::High);
        if (policy.requiresConfirmation) {
            std::cout << "TEMP cleanup will remove eligible entries older than 24 hours. [y/N]: ";
            std::string answer;
            std::getline(std::cin, answer);
            if (answer != "y" && answer != "Y") {
                std::cout << "TEMP cleanup cancelled.\n";
                makeHistory().record("TEMP_CLEANUP_CANCELLED");
                return 0;
            }
        }
    }

    std::cout << (dryRun ? "Previewing: " : "Cleaning: ")
              << health.tempPath << "\n";
    const auto result = handler::cleanTempDirectory(
        std::filesystem::path(health.tempPath), dryRun);
    handler::printCleanupResult(result);
    makeHistory().record("TEMP_CLEANUP",
        "files=" + std::to_string(result.filesRemoved) +
        ", skipped=" + std::to_string(result.skipped) +
        ", dry_run=" + std::string(dryRun ? "true" : "false"));
    return 0;
}

std::filesystem::path transactionRoot() {
    return stateRoot() / "transactions";
}

int runSnapshots() {
    handler::SnapshotStore store(transactionRoot() / "snapshots");
    const auto snapshots = store.list();
    if (snapshots.empty()) {
        std::cout << "No recovery snapshots found.\n";
        return 0;
    }
    std::cout << "Recovery snapshots\n-------------------\n";
    for (const auto& snapshot : snapshots)
        std::cout << snapshot.id << " | " << snapshot.path.string() << '\n';
    return 0;
}

int runRollback(const std::string& id) {
    handler::SnapshotStore store(transactionRoot() / "snapshots");
    const auto snapshot = store.find(id);
    if (!snapshot) {
        std::cerr << "Rollback blocked: snapshot not found: " << id << "\n";
        return 2;
    }
    const auto state = store.load(*snapshot);
    if (!state) {
        std::cerr << "Rollback blocked: snapshot could not be read.\n";
        return 2;
    }

    std::cout << "Rollback target: " << snapshot->id << "\n"
              << "Captured: " << state->timestampUtc << "\n"
              << "Directory: " << state->currentDirectory.string() << "\n"
              << "Note: this restores Handler's saved baseline only; it does not "
                 "silently modify OS environment variables. [y/N]: ";
    std::string answer;
    std::getline(std::cin, answer);
    if (answer != "y" && answer != "Y") {
        std::cout << "Rollback cancelled.\n";
        makeHistory().record("ROLLBACK_CANCELLED", id);
        return 0;
    }

    if (!makeStateStore().saveCurrent(*state)) {
        std::cerr << "Rollback failed: could not restore saved Handler state.\n";
        return 1;
    }
    handler::RecoveryJournal journal(transactionRoot() / "recovery.log");
    journal.record("MANUAL_ROLLBACK", id);
    makeHistory().record("ROLLBACK_APPLIED",
                         id + " | Handler baseline restored");
    std::cout << "Handler baseline restored from snapshot " << id << ".\n";
    return 0;
}

int runStatus() {
    const auto state = makeStateStore().loadCurrent();
    if (!state) {
        std::cout << "Handler status: no saved environment state. Run 'handler state'.\n";
        return 1;
    }
    std::cout << "Handler status\n---------------\n"
              << "Last capture : " << state->timestampUtc << "\n"
              << "Computer     : " << state->computerName << "\n"
              << "User         : " << state->userName << "\n"
              << "TEMP         : " << state->tempPath << "\n"
              << "Directory    : " << state->currentDirectory.string() << "\n"
              << "Version      : " << state->handlerVersion << "\n";
    return 0;
}

int runHistory() {
    const auto file = stateRoot() / "history.log";
    std::ifstream in(file);
    if (!in) {
        std::cout << "No Handler history found.\n";
        return 0;
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    const std::size_t start = lines.size() > 20 ? lines.size() - 20 : 0;
    std::cout << "Handler history (latest " << lines.size() - start << ")\n"
              << "--------------------------------\n";
    for (std::size_t i = start; i < lines.size(); ++i)
        std::cout << lines[i] << '\n';
    return 0;
}

int runState() {
    const auto state = handler::captureEnvironmentState();
    std::cout << handler::formatState(state);
    if (!makeStateStore().saveCurrent(state)) {
        std::cerr << "Handler: failed to persist environment state.\n";
        return 1;
    }
    makeHistory().record("STATE_CAPTURED", "current environment state saved");
    return 0;
}

int runMaintenance() {
    handler::MaintenanceLoop loop(std::chrono::hours(2));
    loop.run([] {
        runTempCleanup();
        runState();
    });
    return 0;
}

int runErrorDetect(const std::string& text) {
    const auto errors = handler::detectErrors(text);
    handler::printDetectedErrors(errors);
    makeHistory().record("ERROR_DETECTION",
        "patterns=" + std::to_string(errors.size()));
    return errors.empty() ? 0 : 1;
}

int runDiscover(const std::vector<std::string>& names) {
    const auto components = handler::discoverComponents(names);
    handler::printComponents(components);
    makeHistory().record("COMPONENT_DISCOVERY",
        "found=" + std::to_string(components.size()));
    return 0;
}

handler::ProjectContext currentProject() {
    return handler::detectProjectContext(std::filesystem::current_path());
}

int runProject() {
    const auto context = currentProject();
    handler::printProjectContext(context);
    makeHistory().record("PROJECT_CONTEXT",
        context.root.empty() ? "not detected" : context.root.string());
    return 0;
}

int runDeps() {
    const auto context = currentProject();
    if (context.root.empty()) {
        std::cout << "No supported project detected in the current directory or its parents.\n";
        return 1;
    }
    const auto info = handler::inspectDependencies(context.root, context.type);
    handler::printDependencies(info);
    makeHistory().record("DEPENDENCY_INSPECTION", info.ecosystem);
    return 0;
}

int runGraph() {
    const auto context = currentProject();
    if (context.root.empty()) {
        std::cout << "No supported project detected.\n";
        return 1;
    }
    const auto info = handler::inspectDependencies(context.root, context.type);
    const auto edges =
        handler::buildDependencyGraph(context.root.filename().string(),
                                       info.declared);
    handler::printDependencyGraph(edges);
    makeHistory().record("DEPENDENCY_GRAPH",
        "edges=" + std::to_string(edges.size()));
    return 0;
}

handler::ModuleRegistry buildModules() {
    handler::ModuleRegistry registry;
    registry.registerModule("help", [] { printUsage(); return 0; });
    registry.registerModule("health", runHealth);
    registry.registerModule("self-check", runSelfCheck);
    registry.registerModule("temp-cleanup", [] { return runTempCleanup(); });
    registry.registerModule("maintenance", runMaintenance);
    registry.registerModule("state", runState);
    registry.registerModule("status", runStatus);
    registry.registerModule("history", runHistory);
    registry.registerModule("version", [] {
        std::cout << "Handler " << kVersion << "\n";
        return 0;
    });
    registry.registerModule("modules", [] {
        std::cout << "Registered modules (activated on demand):\n"
                  << "  health\n  self-check\n  temp-cleanup\n"
                  << "  maintenance\n  state\n  status\n  history\n  version\n  modules\n";
        return 0;
    });
    return registry;
}

} // namespace

int main(int argc, char* argv[]) {
    auto registry = buildModules();
    handler::TaskRouter router(registry);

    if (argc < 2)
        return router.dispatch("help");

    const std::string command = argv[1];

    if (command == "protect") return runProtection();
    if (command == "doctor") return runDoctor();
    if (command == "updates") return runUpdates();
    if (command == "risk") {
        if (argc < 3) return 2;
        return runRisk(joinArguments(argc, argv, 2));
    }
    if (command == "safe-mode") return runSafeMode();
    if (command == "snapshots") return runSnapshots();
    if (command == "rollback") {
        if (argc < 3) { std::cerr << "Usage: handler rollback <snapshot-id>\n"; return 2; }
        return runRollback(argv[2]);
    }

    if (command == "recover") {
        if (argc < 3) {
            std::cerr << "Usage: handler recover <error text>\n";
            return 2;
        }
        return handler::autoRecover(joinArguments(argc, argv, 2));
    }

    if (command == "repair") {
        if (argc < 4) {
            std::cerr << "Usage: handler repair <python-module|node-module> <package>\n";
            return 2;
        }
        if (std::string(argv[2]) == "python-module")
            return handler::repairPythonModule(argv[3]);
        if (std::string(argv[2]) == "node-module")
            return handler::repairNodeModule(argv[3]);
        std::cerr << "Unknown repair target: " << argv[2] << "\n";
        return 2;
    }

    if (command == "temp-cleanup" && argc >= 3 &&
        std::string(argv[2]) == "--dry-run")
        return runTempCleanup(true);

    if (command == "decide") {
        if (argc < 3) {
            std::cerr << "Usage: handler decide <error text>\n";
            return 2;
        }
        return runDecide(joinArguments(argc, argv, 2));
    }

    if (command == "command") {
        if (argc < 3) {
            std::cerr << "Usage: handler command <tool> [args...]\n";
            return 2;
        }
        std::vector<std::string> args;
        for (int i = 3; i < argc; ++i)
            args.emplace_back(argv[i]);
        return runCommand(argv[2], args);
    }

    if (command == "detect-error") {
        if (argc < 3) {
            std::cerr << "Usage: handler detect-error <error text>\n";
            return 2;
        }
        return runErrorDetect(joinArguments(argc, argv, 2));
    }

    if (command == "discover") {
        std::vector<std::string> names;
        for (int i = 2; i < argc; ++i)
            names.emplace_back(argv[i]);
        if (names.empty())
            names = {"python", "node", "git", "cmake", "dotnet",
                     "java", "go", "cargo"};
        return runDiscover(names);
    }

    if (command == "project") return runProject();
    if (command == "deps") return runDeps();
    if (command == "graph") return runGraph();

    const int result = router.dispatch(command);
    if (result == 127) {
        std::cerr << "Unknown Handler command: " << command << "\n\n";
        printUsage();
        return 2;
    }
    return result;
}
