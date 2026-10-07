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
#include "handler/transaction.h"

#include "handler/component_discovery.h"
#include "handler/system_info.h"
#include "handler/uninstall.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

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

    Transaction rollbackTx(SafetyMode::Confirm);
    const auto rollbackOk = rollbackTx.runApproved(
        RiskLevel::Low,
        [] { return false; },
        [] { return VerificationResult{true, "not reached", ""}; },
        [] { return true; });
    assert(!rollbackOk.committed && rollbackOk.rolledBack);

    Transaction rollbackFailTx(SafetyMode::Confirm);
    const auto rollbackFail = rollbackFailTx.runApproved(
        RiskLevel::Low,
        [] { return false; },
        [] { return VerificationResult{true, "not reached", ""}; },
        [] { return false; });
    assert(!rollbackFail.committed && !rollbackFail.rolledBack);

    const auto low = evaluatePolicy(SafetyMode::Confirm, RiskLevel::Low);
    assert(low.allowed && !low.requiresConfirmation);

    const auto high = evaluatePolicy(SafetyMode::Confirm, RiskLevel::High);
    assert(!high.allowed && high.requiresConfirmation);

    const auto safeRisk = classifyCommandRisk("python", {"--version"});
    assert(safeRisk == RiskLevel::Low);
    const auto installRisk = classifyCommandRisk("npm", {"install", "express"});
    assert(installRisk == RiskLevel::High);

    const auto invalidDependencyRoot =
        std::filesystem::temp_directory_path() / "handler-invalid-dependency-test";
    std::filesystem::remove_all(invalidDependencyRoot, ec);
    std::filesystem::create_directories(invalidDependencyRoot, ec);
    assert(upgradeDependency(invalidDependencyRoot, "Python", "bad;package", ">=1.0") == 3);
    assert(upgradeDependency(invalidDependencyRoot, "Node.js", "bad;package", ">=1.0") == 3);
    assert(upgradeDependency(invalidDependencyRoot, "Unknown", "package", ">=1.0") == 3);
    std::filesystem::remove_all(invalidDependencyRoot, ec);

    const auto errors = detectErrors("ModuleNotFoundError: No module named 'requests'");
    assert(!errors.empty());

    DependencyInfo depInfo;
    depInfo.ecosystem = "Python";
    depInfo.manifest = "requirements.txt";
    depInfo.declared = {"requests>=3.0", "flask==3.0", "requests<3.0"};
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
    assert(dependencyConstraintsCompatible({">=21.0,<22.0"}));
    assert(!dependencyConstraintsCompatible({">=21.0,<21.0"}));
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
    state.handlerVersion = "0.8.0";
    const auto snapshot = snapshots.create(state);
    assert(snapshot.has_value());
    assert(snapshots.find(snapshot->id).has_value());
    assert(snapshots.load(*snapshot).has_value());
    assert(snapshots.list().size() == 1);
    assert(!snapshots.find("../outside").has_value());
    SnapshotInfo traversal{"../outside", tempRoot / ".." / "outside.state"};
    assert(!snapshots.load(traversal).has_value());
    std::filesystem::remove_all(tempRoot, ec);

    const auto testStateRoot = handlerTransactionRoot() / "tests";
    std::filesystem::create_directories(testStateRoot, ec);
    const auto pathBaseline = testStateRoot / "path.baseline";
    assert(isHandlerStatePath(pathBaseline));
    assert(savePathBaseline(pathBaseline));
    std::vector<std::string> loaded;
    assert(loadPathBaseline(pathBaseline, loaded));
    assert(!loaded.empty());

    const auto outsideBaseline = std::filesystem::temp_directory_path() / "handler-untrusted.baseline";
    assert(!isHandlerStatePath(outsideBaseline));
    assert(!loadPathBaseline(outsideBaseline, loaded));

    const auto outsideEnvBaseline = std::filesystem::temp_directory_path() / "handler-untrusted.environment.baseline";
    std::vector<EnvironmentEntry> envEntries;
    assert(!loadEnvironmentBaseline(outsideEnvBaseline, envEntries));

    const auto envFile = testStateRoot / "environment.baseline";
#ifdef _WIN32
    assert(saveEnvironmentBaseline(envFile, {"PATH", "TEMP"}));
#else
    assert(saveEnvironmentBaseline(envFile, {"PATH", "HOME"}));
#endif
    assert(loadEnvironmentBaseline(envFile, envEntries));
    assert(!envEntries.empty());
    const auto envDiff = compareEnvironmentBaseline(envEntries);
    assert(envDiff.missing.empty());
    assert(isSensitiveVariable("API_TOKEN"));
    assert(isSensitiveVariable("NORMAL_VALUE") == false);
    std::filesystem::remove_all(testStateRoot, ec);

    const auto doctor = inspectToolchain({"python", "definitely-not-a-handler-tool"});
    assert(doctor.size() == 2);
    assert(!doctor[1].available);
    assert(doctor[1].status == "MISSING");
    const auto candidates = proposeToolchainRepairs(doctor);
    assert(candidates.size() == 2);
    std::string repairDetails;
    assert(!repairToolchain("java", repairDetails));
    assert(!repairDetails.empty());
#ifndef _WIN32
    repairDetails.clear();
    assert(!repairToolchain("python", repairDetails));
    assert(repairDetails.find("not enabled") != std::string::npos);
#endif

    const auto uninstallRoot = std::filesystem::temp_directory_path() / "handler_uninstall_test";
    std::filesystem::remove_all(uninstallRoot, ec);
    std::filesystem::create_directories(uninstallRoot);
    {
        std::ofstream manifest(uninstallRoot / "requirements.txt");
        manifest << "handler-test-package-that-does-not-exist-987654>=1.0\n";
    }
    const auto blockedPlan = planUninstall(
        uninstallRoot, UninstallEcosystem::Python, "handler-test-package-that-does-not-exist-987654");
    assert(blockedPlan.allowed == false);
    assert(!blockedPlan.reason.empty());
    const auto pythonScopePlan = planUninstall(
        uninstallRoot, UninstallEcosystem::Python, "handler-test-package-that-does-not-exist-987654");
    assert(!pythonScopePlan.allowed);
    assert(pythonScopePlan.reason.find("project-local") != std::string::npos);

    std::filesystem::create_directories(uninstallRoot / "node_modules");
    {
        std::ofstream packageJson(uninstallRoot / "package.json");
        packageJson << "{\"dependencies\":{\"handler-test-package-that-does-not-exist-987654\":\"1.0.0\"}}\n";
    }
    const auto nodeScopePlan = planUninstall(
        uninstallRoot, UninstallEcosystem::NodeJs, "handler-test-package-that-does-not-exist-987654");
    assert(!nodeScopePlan.allowed);
    assert(nodeScopePlan.reason.find("not currently installed") != std::string::npos ||
           nodeScopePlan.reason.find("not a direct") != std::string::npos);

    const auto invalidPlan = planUninstall(
        uninstallRoot, UninstallEcosystem::Python, "bad;package");
    assert(!invalidPlan.allowed);
    assert(invalidPlan.reason.find("invalid") != std::string::npos);
    const auto missingManifestRoot = std::filesystem::temp_directory_path() / "handler_uninstall_missing_manifest";
    std::filesystem::remove_all(missingManifestRoot, ec);
    std::filesystem::create_directories(missingManifestRoot / "node_modules");
    const auto missingManifestPlan = planUninstall(
        missingManifestRoot, UninstallEcosystem::NodeJs, "left-pad");
    assert(!missingManifestPlan.allowed);
    assert(missingManifestPlan.reason.find("manifest") != std::string::npos);
    std::filesystem::remove_all(missingManifestRoot, ec);

    const auto malformedRoot = std::filesystem::temp_directory_path() / "handler_uninstall_malformed";
    std::filesystem::remove_all(malformedRoot, ec);
    std::filesystem::create_directories(malformedRoot);
    std::ofstream malformed(malformedRoot / "package.json");
    malformed << "{not-json";
    malformed.close();
    std::filesystem::create_directories(malformedRoot / "node_modules");
    const auto malformedPlan = planUninstall(
        malformedRoot, UninstallEcosystem::NodeJs, "left-pad");
    assert(!malformedPlan.allowed);
    std::filesystem::remove_all(malformedRoot, ec);

    std::filesystem::remove_all(uninstallRoot, ec);

    std::cout << "Handler core tests passed.\n";
    return 0;
}
