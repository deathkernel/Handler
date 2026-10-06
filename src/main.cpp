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

constexpr const char* kVersion = "0.7.0";

std::filesystem::path stateRoot() {
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData && *localAppData) return std::filesystem::path(localAppData) / "Handler";
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile && *userProfile) return std::filesystem::path(userProfile) / ".handler";
    return std::filesystem::current_path() / ".handler";
}

int runUpdates() { for (const auto& u : handler::inspectToolUpdates({"python","node","git","cmake","dotnet"})) std::cout << u.tool << ": " << (u.currentPath.empty() ? "missing" : u.currentPath) << " | " << u.updateHint << "
"; return 0; }

int runRisk(const std::string& line) { const auto r = handler::inspectCommandRisk(line); std::cout << "Risk: " << (r.risk == handler::CommandRisk::Safe ? "SAFE" : r.risk == handler::CommandRisk::Review ? "REVIEW" : "BLOCKED") << " | " << r.reason << "
"; return r.risk == handler::CommandRisk::Blocked ? 3 : 0; }

int runSafeMode() { const auto c = handler::createSafeModeContext(std::filesystem::current_path()); std::cout << "Safe mode: " << (c.enabled ? "ready" : "failed") << "
Sandbox: " << c.root << "
"; return c.enabled ? 0 : 1; }

int runProtection() { handler::printEnvironmentHealth(handler::inspectEnvironmentHealth()); for (const auto& f : handler::inspectPathEntries()) std::cout << "[" << (f.exists ? "OK" : "WARN") << "] PATH " << f.entry << ": " << f.details << "
"; const auto vars = handler::inspectEnvironmentVariables({"TEMP","PATH","USERPROFILE","LOCALAPPDATA"}); for (const auto& v : vars) std::cout << "[" << (v.present ? "OK" : "WARN") << "] ENV " << v.name << ": " << v.details << "
"; const auto ports = handler::inspectPorts({3000,5000,8000,8080}); for (const auto& p : ports) std::cout << "[" << (p.available ? "OK" : "BUSY") << "] PORT " << p.port << ": " << p.details << "
"; const auto tools = handler::inspectToolchain({"python","git","node","cmake","dotnet"}); for (const auto& t : tools) std::cout << "[" << (t.available ? "OK" : "WARN") << "] TOOL " << t.tool << ": " << t.details << "
"; handler::History(std::filesystem::path(stateRoot()) / "history.log").record("PROTECTION_CHECK","environment/path/variables/ports/toolchain inspected"); return 0; }

void printDecisions(const std::vector<handler::Decision>& decisions) { for (const auto& d : decisions) std::cout << d.action << " | risk=" << static_cast<int>(d.risk) << " | " << d.reason << "
"; }

int runDecide(const std::string& text) { const auto errors = handler::detectErrors(text); const auto decisions = handler::decideRepairs(errors); printDecisions(decisions); handler::History(std::filesystem::path(stateRoot()) / "history.log").record("DECISION", "actions=" + std::to_string(decisions.size())); return decisions.empty() ? 0 : 1; }

int runCommand(const std::string& executable, const std::vector<std::string>& args) { handler::CommandSpec spec{"cli-command", executable, args, handler::RiskLevel::Medium}; std::cout << handler::buildCommandLine(spec) << "
"; const auto policy = handler::evaluatePolicy(handler::SafetyMode::Confirm, spec.risk); if (!policy.allowed) { std::cout << "Command blocked: " << policy.reason << "
"; return 3; } const auto result = handler::executeCommand(spec); std::cout << result.output; return result.started ? result.exitCode : 1; }

void printUsage() {
    std::cout
        << "Handler - Developer Environment Guardian

"
        << "Usage:
"
        << "  handler health                 Inspect PC/system environment
"
        << "  handler self-check             Run Handler's own checks
"
        << "  handler temp-cleanup           Safely clean %TEMP%
"
        << "  handler maintenance            Run maintenance every 2 hours
"
        << "  handler state                  Capture/show current environment state
"
        << "  handler detect-error <text>    Detect known error patterns
"
        << "  handler discover [tools...]    Discover tools on PATH
"
        << "  handler project                Detect project context
"
        << "  handler deps                   Inspect project dependencies
"
        << "  handler graph                  Show discovered dependency edges
"
        << "  handler decide <error text>    Generate deterministic repair decisions
"
        << "  handler command <tool> [...]   Execute an allowlisted command
"
        << "  handler protect                 Run Level 5 PC protection diagnostics
"
        << "  handler updates                 Inspect tool update candidates
"
        << "  handler risk <command>          Intercept risky command patterns
"
        << "  handler safe-mode               Prepare isolated sandbox context
"
        << "  handler modules                Show registered on-demand modules
"
        << "  handler version                Show Handler version
"
        << "  handler help                   Show this help
";
}

handler::StateStore makeStateStore() { return handler::StateStore(stateRoot() / "state"); }
handler::History handler::History(std::filesystem::path(stateRoot()) / "history.log") { return handler::History(stateRoot() / "history.log"); }

int runHealth() {
    const auto health = handler::inspectSystem();
    handler::printSystemHealth(health);
    const auto state = handler::captureEnvironmentState();
    if (!makeStateStore().saveCurrent(state))
        std::cerr << "[WARN] Could not persist environment state.
";
    handler::History(std::filesystem::path(stateRoot()) / "history.log").record("HEALTH_CHECK", "system observation completed");
    return 0;
}

int runSelfCheck() {
    const auto health = handler::inspectSystem();
    bool ok = true;
    std::cout << "Handler Self-Check
------------------
";
    if (health.tempAvailable) std::cout << "[OK] TEMP environment variable
";
    else { std::cout << "[FAIL] TEMP environment variable
"; ok = false; }
    if (health.pathAvailable) std::cout << "[OK] PATH environment variable
";
    else std::cout << "[WARN] PATH environment variable is missing
";
    if (makeStateStore().saveCurrent(handler::captureEnvironmentState()))
        std::cout << "[OK] Environment state store
";
    else { std::cout << "[FAIL] Environment state store
"; ok = false; }
    handler::History(std::filesystem::path(stateRoot()) / "history.log").record("SELF_CHECK", ok ? "passed" : "failed");
    std::cout << (ok ? "
Self-check passed.
" : "
Self-check found a required problem.
");
    return ok ? 0 : 1;
}

int runTempCleanup() {
    const auto health = handler::inspectSystem();
    if (!health.tempAvailable) {
        std::cerr << "Handler: TEMP is not available. Cleanup skipped.
";
        handler::History(std::filesystem::path(stateRoot()) / "history.log").record("TEMP_CLEANUP_SKIPPED", "TEMP unavailable");
        return 1;
    }
    std::cout << "Cleaning: " << health.tempPath << '
';
    const auto result = handler::cleanTempDirectory(std::filesystem::path(health.tempPath));
    handler::printCleanupResult(result);
    handler::History(std::filesystem::path(stateRoot()) / "history.log").record("TEMP_CLEANUP",
        "files=" + std::to_string(result.filesRemoved) +
        ", skipped=" + std::to_string(result.skipped));
    return 0;
}

int runState() {
    const auto state = handler::captureEnvironmentState();
    std::cout << handler::formatState(state);
    if (!makeStateStore().saveCurrent(state)) {
        std::cerr << "Handler: failed to persist environment state.
";
        return 1;
    }
    handler::History(std::filesystem::path(stateRoot()) / "history.log").record("STATE_CAPTURED", "current environment state saved");
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
    handler::History(std::filesystem::path(stateRoot()) / "history.log").record("ERROR_DETECTION", "patterns=" + std::to_string(errors.size()));
    return errors.empty() ? 0 : 1;
}

int runDiscover(const std::vector<std::string>& names) {
    const auto components = handler::discoverComponents(names);
    handler::printComponents(components);
    handler::History(std::filesystem::path(stateRoot()) / "history.log").record("COMPONENT_DISCOVERY", "found=" + std::to_string(components.size()));
    return 0;
}

handler::ProjectContext currentProject() {
    return handler::detectProjectContext(std::filesystem::current_path());
}

int runProject() {
    const auto context = currentProject();
    handler::printProjectContext(context);
    handler::History(std::filesystem::path(stateRoot()) / "history.log").record("PROJECT_CONTEXT", context.root.empty() ? "not detected" : context.root.string());
    return 0;
}

int runDeps() {
    const auto context = currentProject();
    if (context.root.empty()) {
        std::cout << "No supported project detected in the current directory or its parents.
";
        return 1;
    }
    const auto info = handler::inspectDependencies(context.root, context.type);
    handler::printDependencies(info);
    handler::History(std::filesystem::path(stateRoot()) / "history.log").record("DEPENDENCY_INSPECTION", info.ecosystem);
    return 0;
}

int runGraph() {
    const auto context = currentProject();
    if (context.root.empty()) {
        std::cout << "No supported project detected.
";
        return 1;
    }
    const auto info = handler::inspectDependencies(context.root, context.type);
    const auto edges = handler::buildDependencyGraph(context.root.filename().string(), info.declared);
    handler::printDependencyGraph(edges);
    handler::History(std::filesystem::path(stateRoot()) / "history.log").record("DEPENDENCY_GRAPH", "edges=" + std::to_string(edges.size()));
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
    registry.registerModule("version", [] { std::cout << "Handler " << kVersion << '
'; return 0; });
    registry.registerModule("modules", [] {
        std::cout << "Registered modules (activated on demand):
"
                  << "  health
  self-check
  temp-cleanup
  maintenance
  state
"
                  << "  detect-error
  discover
  project
  deps
  graph
  version
  modules
";
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

    if (command == "protect") return runProtection();
    if (command == "updates") return runUpdates();
    if (command == "risk") { if (argc < 3) return 2; return runRisk(argv[2]); }
    if (command == "safe-mode") return runSafeMode();

    if (command == "decide") { if (argc < 3) { std::cerr << "Usage: handler decide <error text>
"; return 2; } return runDecide(argv[2]); }

    if (command == "command") { if (argc < 3) { std::cerr << "Usage: handler command <tool> [args...]
"; return 2; } std::vector<std::string> args; for (int i = 3; i < argc; ++i) args.emplace_back(argv[i]); return runCommand(argv[2], args); }

    if (command == "detect-error") {
        if (argc < 3) {
            std::cerr << "Usage: handler detect-error <error text>
";
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
        std::cerr << "Unknown Handler command: " << command << "

";
        printUsage();
        return 2;
    }
    return result;
}
