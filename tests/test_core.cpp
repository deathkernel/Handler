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
#include "handler/artifact_backup.h"
#include "handler/state_paths.h"

#include "handler/component_discovery.h"
#include "handler/system_info.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>

int main() {
    using namespace handler;

    assert(quoteArgument("plain") == "plain");
    assert(quoteArgument("hello world") == "\"hello world\"");
    assert(quoteArgument("a\\b c") == "\"a\\b c\"");

    const auto stateRoot = handlerStateRoot();
    const auto transactionRoot = handlerTransactionRoot();
    assert(transactionRoot == stateRoot / "transactions");

    const auto backupBase = std::filesystem::temp_directory_path() / "handler-batch2-test";
    std::error_code testEc;
    std::filesystem::remove_all(backupBase, testEc);
    std::filesystem::create_directories(backupBase, testEc);
    assert(!testEc);
    const auto original = backupBase / "manifest.txt";
    {
        std::ofstream out(original);
        out << "handler-batch-2";
    }
    const auto backup = backupArtifact(original, backupBase / "backups");
    assert(backup.has_value());
    assert(backup->existed && backup->originalSize == backup->backupSize);
    {
        std::ofstream out(original, std::ios::trunc);
        out << "corrupted";
    }
    assert(restoreArtifact(*backup));
    std::ifstream restored(original);
    std::string restoredText;
    std::getline(restored, restoredText);
    assert(restoredText == "handler-batch-2");
    std::filesystem::remove_all(backupBase, testEc);

    const auto discovered = discoverComponents({"python", "definitely-not-a-handler-tool"});
    assert(discovered.size() <= 1);
    for (const auto& component : discovered) {
        assert(component.executable);
        assert(!component.path.empty());
        assert(std::filesystem::is_regular_file(component.path));
    }

    const auto systemHealth = inspectSystem();
    assert(!systemHealth.pathAvailable || !systemHealth.pathValue.empty());
    assert(systemHealth.tempAvailable);

    assert(isAllowedExecutable("python"));
    assert(isAllowedExecutable("dotnet"));
    assert(isAllowedExecutable("npm"));
    assert(!isAllowedExecutable("format"));
    assert(isAllowedExecutable("winget"));
    assert(classifyCommandRisk("winget", {"upgrade", "--id", "Git.Git"}) == RiskLevel::High);

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

    const auto v250 = parseDependencyVersion("2.5.0");
    const auto v350 = parseDependencyVersion("3.5.0");
    assert(v250.has_value() && v350.has_value());
    assert(satisfiesDependencyConstraint(*v250, ">=2.0,<3.0"));
    assert(!satisfiesDependencyConstraint(*v350, ">=2.0,<3.0"));
    assert(dependencyConstraintsCompatible({">=2.0", "<3.0"}));
    assert(!dependencyConstraintsCompatible({">=3.0", "<3.0"}));
    const auto selected = selectCompatibleDependencyVersion(
        {">=2.0,<4.0"}, {"1.9.0", "2.4.0", "3.1.0", "4.0.0"});
    assert(selected.has_value() && *selected == "3.1.0");
    const auto caret = selectCompatibleDependencyVersion(
        {"^2.1.0"}, {"2.0.0", "2.1.0", "2.9.0", "3.0.0"});
    assert(caret.has_value() && *caret == "2.9.0");

    const auto graph = buildDependencyGraph("demo", {"requests", "flask"});
    assert(graph.size() == 2);
    assert(graph[0].source == "demo");
    const std::vector<DependencyEdge> transitive = buildTransitiveDependencyGraph(
        graph, {{"flask", "werkzeug", "transitive"}, {"demo", "requests", "declares"}});
    assert(transitive.size() == 4);
    const std::vector<DependencyImpact> werkzeugImpact = analyzeDependencyImpact(transitive, "werkzeug");
    assert(werkzeugImpact.size() == 1);
    assert(werkzeugImpact[0].affected.size() == 2);
    assert(werkzeugImpact[0].risk == "HIGH");
    const std::vector<DependencyImpact> impact = analyzeDependencyImpact(transitive, "requests");
    assert(impact.size() == 1);
    assert(impact[0].risk == "MEDIUM");
    assert(impact[0].affected.size() == 1);

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
    assert(!repairToolchain("java", repairDetails));
    assert(repairDetails.find("winget") != std::string::npos ||
           repairDetails.find("Windows-only") != std::string::npos ||
           repairDetails.find("available") != std::string::npos);
#ifndef _WIN32
    repairDetails.clear();
    assert(!repairToolchain("python", repairDetails));
    assert(repairDetails.find("not enabled") != std::string::npos);
#endif

    std::cout << "Handler core tests passed.\n";
    return 0;
}
