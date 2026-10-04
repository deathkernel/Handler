#include "handler/environment_health.h"
#include "handler/path_guardian.h"
#include "handler/environment_guardian.h"
#include "handler/resource_guardian.h"
#include "handler/toolchain_doctor.h"
#include "handler/action_engine.h"
#include "handler/circuit_breaker.h"
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

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr const char* kVersion = "0.5.0";

std::filesystem::path stateRoot() {
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData && *localAppData) return std::filesystem::path(localAppData) / "Handler";
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile && *userProfile) return std::filesystem::path(userProfile) / ".handler";
    return std::filesystem::current_path() / ".handler";
}

int runProtection() { handler::printEnvironmentHealth(handler::inspectEnvironmentHealth()); for (const auto& f : handler::inspectPathEntries()) std::cout << "[" << (f.exists ? "OK" : "WARN") << "] PATH " << f.entry << ": " << f.details << "\n"; const auto vars = handler::inspectEnvironmentVariables({"TEMP","PATH","USERPROFILE","LOCALAPPDATA"}); for (const auto& v : vars) std::cout << "[" << (v.present ? "OK" : "WARN") << "] ENV " << v.name << ": " << v.details << "\n"; const auto ports = handler::inspectPorts({3000,5000,8000,8080}); for (const auto& p : ports) std::cout << "[" << (p.available ? "OK" : "BUSY") << "] PORT " << p.port << ": " << p.details << "\n"; const auto tools = handler::inspectToolchain({"python","git","node","cmake","dotnet"}); for (const auto& t : tools) std::cout << "[" << (t.available ? "OK" : "WARN") << "] TOOL " << t.tool << ": " << t.details << "\n"; makeHistory().record("PROTECTION_CHECK","environment/path/variables/ports/toolchain inspected"); return 0; }\n\nvoid printDecisions(const std::vector<handler::Decision>& decisions) { for (const auto& d : decisions) std::cout << d.action << " | risk=" << static_cast<int>(d.risk) << " | " << d.reason << "\n"; }\n\nint runDecide(const std::string& text) { const auto errors = handler::detectErrors(text); const auto decisions = handler::decideRepairs(errors); printDecisions(decisions); makeHistory().record("DECISION", "actions=" + std::to_string(decisions.size())); return decisions.empty() ? 0 : 1; }\n\nint runCommand(const std::string& executable, const std::vector<std::string>& args) { handler::CommandSpec spec{"cli-command", executable, args, handler::RiskLevel::Medium}; std::cout << handler::buildCommandLine(spec) << "\n"; const auto policy = handler::evaluatePolicy(handler::SafetyMode::Confirm, spec.risk); if (!policy.allowed) { std::cout << "Command blocked: " << policy.reason << "\n"; return 3; } const auto result = handler::executeCommand(spec); std::cout << result.output; return result.started ? result.exitCode : 1; }\n\nvoid printUsage() {
    std::cout
        << "Handler - Developer Environment Guardian\n\n"
        << "Usage:\n"
        << "  handler health                 Inspect PC/system environment\n"
        << "  handler self-check             Run Handler's own checks\n"
        << "  handler temp-cleanup           Safely clean %TEMP%\n"
        << "  handler maintenance            Run maintenance every 2 hours\n"
        << "  handler state                  Capture/show current environment state\n"
        << "  handler detect-error <text>    Detect known error patterns\n"
        << "  handler discover [tools...]    Discover tools on PATH\n"
        << "  handler project                Detect project context\n"
        << "  handler deps                   Inspect project dependencies\n"
        << "  handler graph                  Show discovered dependency edges\n"
        << "  handler decide <error text>    Generate deterministic repair decisions\n"
        << "  handler command <tool> [...]   Execute an allowlisted command\n"
        << "  handler protect                 Run Level 5 PC protection diagnostics\n"
        << "  handler modules                Show registered on-demand modules\n"
        << "  handler version                Show Handler version\n"
        << "  handler help                   Show this help\n";
}

handler::StateStore makeStateStore() { return handler::StateStore(stateRoot() / "state"); }
handler::History makeHistory() { return handler::History(stateRoot() / "history.log"); }

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
    if (health.tempAvailable) std::cout << "[OK] TEMP environment variable\n";
    else { std::cout << "[FAIL] TEMP environment variable\n"; ok = false; }
    if (health.pathAvailable) std::cout << "[OK] PATH environment variable\n";
    else std::cout << "[WARN] PATH environment variable is missing\n";
    if (makeStateStore().saveCurrent(handler::captureEnvironmentState()))
        std::cout << "[OK] Environment state store\n";
    else { std::cout << "[FAIL] Environment state store\n"; ok = false; }
    makeHistory().record("SELF_CHECK", ok ? "passed" : "failed");
    std::cout << (ok ? "\nSelf-check passed.\n" : "\nSelf-check found a required problem.\n");
    return ok ? 0 : 1;
}

int runTempCleanup() {
    const auto health = handler::inspectSystem();
    if (!health.tempAvailable) {
        std::cerr << "Handler: TEMP is not available. Cleanup skipped.\n";
        makeHistory().record("TEMP_CLEANUP_SKIPPED", "TEMP unavailable");
        return 1;
    }
    std::cout << "Cleaning: " << health.tempPath << '\n';
    const auto result = handler::cleanTempDirectory(std::filesystem::path(health.tempPath));
    handler::printCleanupResult(result);
    makeHistory().record("TEMP_CLEANUP",
        "files=" + std::to_string(result.filesRemoved) +
        ", skipped=" + std::to_string(result.skipped));
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
    loop.run([] { runTempCleanup(); runState(); });
    return 0;
}

int runErrorDetect(const std::string& text) {
    const auto errors = handler::detectErrors(text);
    handler::printDetectedErrors(errors);
    makeHistory().record("ERROR_DETECTION", "patterns=" + std::to_string(errors.size()));
    return errors.empty() ? 0 : 1;
}

int runDiscover(const std::vector<std::string>& names) {
    const auto components = handler::discoverComponents(names);
    handler::printComponents(components);
    makeHistory().record("COMPONENT_DISCOVERY", "found=" + std::to_string(components.size()));
    return 0;
}

handler::ProjectContext currentProject() {
    return handler::detectProjectContext(std::filesystem::current_path());
}

int runProject() {
    const auto context = currentProject();
    handler::printProjectContext(context);
    makeHistory().record("PROJECT_CONTEXT", context.root.empty() ? "not detected" : context.root.string());
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
    const auto edges = handler::buildDependencyGraph(context.root.filename().string(), info.declared);
    handler::printDependencyGraph(edges);
    makeHistory().record("DEPENDENCY_GRAPH", "edges=" + std::to_string(edges.size()));
    return 0;
}

handler::ModuleRegistry buildModules() {
    handler::ModuleRegistry registry;
    registry.registerModule("help", [] { printUsage(); return 0; });
    registry.registerModule("health", runHealth);
    registry.registerModule("self-check", runSelfCheck);
    registry.registerModule("temp-cleanup", runTempCleanup);
    registry.registerModule("maintenance", runMaintenance);
    registry.registerModule("state", runState);
    registry.registerModule("version", [] { std::cout << "Handler " << kVersion << '\n'; return 0; });
    registry.registerModule("modules", [] {
        std::cout << "Registered modules (activated on demand):\n"
                  << "  health\n  self-check\n  temp-cleanup\n  maintenance\n  state\n"
                  << "  detect-error\n  discover\n  project\n  deps\n  graph\n  version\n  modules\n";
        return 0;
    });
    return registry;
}

} // namespace

int main(int argc, char* argv[]) {
    auto registry = buildModules();
    handler::TaskRouter router(registry);

    if (argc < 2) return router.dispatch("help");

    const std::string command = argv[1];

    if (command == "protect") return runProtection();\n\n    if (command == "decide") { if (argc < 3) { std::cerr << "Usage: handler decide <error text>\n"; return 2; } return runDecide(argv[2]); }\n\n    if (command == "command") { if (argc < 3) { std::cerr << "Usage: handler command <tool> [args...]\n"; return 2; } std::vector<std::string> args; for (int i = 3; i < argc; ++i) args.emplace_back(argv[i]); return runCommand(argv[2], args); }\n\n    if (command == "detect-error") {
        if (argc < 3) {
            std::cerr << "Usage: handler detect-error <error text>\n";
            return 2;
        }
        return runErrorDetect(argv[2]);
    }

    if (command == "discover") {
        std::vector<std::string> names;
        for (int i = 2; i < argc; ++i) names.emplace_back(argv[i]);
        if (names.empty()) names = {"python", "node", "git", "cmake", "dotnet", "java", "go", "cargo"};
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
