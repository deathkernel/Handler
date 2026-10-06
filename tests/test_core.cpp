#include "handler/command_engine.h"
#include "handler/dependency_graph.h"
#include "handler/error_detection.h"
#include "handler/policy.h"
#include "handler/risky_command.h"
#include "handler/snapshot.h"
#include "handler/path_guardian.h"
#include "handler/environment_guardian.h"
#include "handler/toolchain_doctor.h"
#include "handler/dependency_manager.h"

#include <cassert>
#include <iostream>
#include <filesystem>

int main() {
    using namespace handler;

    assert(quoteArgument("plain") == "plain");
    assert(quoteArgument("hello world") == "\"hello world\"");
    assert(quoteArgument("a\\b c") == "\"a\\\\b c\"");

    assert(isAllowedExecutable("python"));
    assert(isAllowedExecutable("dotnet"));
    assert(isAllowedExecutable("npm"));
    assert(!isAllowedExecutable("format"));

    const auto low = evaluatePolicy(SafetyMode::Confirm, RiskLevel::Low);
    assert(low.allowed && !low.requiresConfirmation);

    const auto high = evaluatePolicy(SafetyMode::Confirm, RiskLevel::High);
    assert(!high.allowed && high.requiresConfirmation);

    const auto safeRisk = classifyCommandRisk("python", {"--version"});
    assert(safeRisk == RiskLevel::Low);
    const auto installRisk = classifyCommandRisk("npm", {"install", "express"});
    assert(installRisk == RiskLevel::High);


    const auto errors = detectErrors("ModuleNotFoundError: No module named 'requests'");
    assert(!errors.empty());

    DependencyInfo depInfo;
    depInfo.ecosystem = "Python";
    depInfo.manifest = "requirements.txt";
    depInfo.declared = {"requests>=2.0", "flask==3.0", "requests<3.0"};
    const auto requirements = parseDependencyRequirements(depInfo);
    assert(requirements.size() == 3);
    const auto conflicts = findDependencyConflicts(requirements);
    assert(conflicts.size() == 1);
    assert(conflicts[0].name == "requests");
    const auto upgradeCandidates = proposeDependencyUpgrades(requirements);
    assert(upgradeCandidates.size() == 3);

    const auto graph = buildDependencyGraph("demo", {"requests", "flask"});
    assert(graph.size() == 2);
    assert(graph[0].source == "demo");

    const auto tempRoot = std::filesystem::temp_directory_path() / "handler_snapshot_test";
    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
    SnapshotStore snapshots(tempRoot);
    EnvironmentState state;
    state.timestampUtc = "test";
    state.computerName = "machine";
    state.userName = "user";
    state.handlerVersion = "0.7.0";
    const auto snapshot = snapshots.create(state);
    assert(snapshot.has_value());
    assert(snapshots.find(snapshot->id).has_value());
    assert(snapshots.load(*snapshot).has_value());
    assert(snapshots.list().size() == 1);
    std::filesystem::remove_all(tempRoot, ec);

    const auto pathRoot = std::filesystem::temp_directory_path() / "handler_path_test";
    std::filesystem::remove_all(pathRoot, ec);
    const auto baseline = pathRoot / "path.baseline";
    assert(savePathBaseline(baseline));
    std::vector<std::string> loaded;
    assert(loadPathBaseline(baseline, loaded));
    assert(!loaded.empty());
    std::filesystem::remove_all(pathRoot, ec);

    const auto envRoot = std::filesystem::temp_directory_path() / "handler_env_test";
    std::filesystem::remove_all(envRoot, ec);
    const auto envFile = envRoot / "environment.baseline";
    assert(saveEnvironmentBaseline(envFile, {"PATH", "TEMP"}));
    std::vector<EnvironmentEntry> envEntries;
    assert(loadEnvironmentBaseline(envFile, envEntries));
    assert(!envEntries.empty());
    const auto envDiff = compareEnvironmentBaseline(envEntries);
    assert(envDiff.missing.empty());
    assert(!isSensitiveVariable("API_TOKEN"));
    assert(isSensitiveVariable("NORMAL_VALUE") == false);
    std::filesystem::remove_all(envRoot, ec);

    const auto doctor = inspectToolchain({"python", "definitely-not-a-handler-tool"});
    assert(doctor.size() == 2);
    assert(!doctor[1].available);
    assert(doctor[1].status == "MISSING");
    const auto candidates = proposeToolchainRepairs(doctor);
    assert(candidates.size() == 2);
    std::string repairDetails;
    assert(!repairToolchain("node", repairDetails));
    assert(repairDetails.find("explicit installer/source") != std::string::npos);

    std::cout << "Handler core tests passed.\n";
    return 0;
}
