#include "handler/command_engine.h"
#include "handler/action_engine.h"
#include "handler/executable_trust.h"
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
#include "handler/recovery_journal.h"
#include "handler/transaction_lock.h"

#include "handler/component_discovery.h"
#include "handler/system_info.h"
#include "handler/uninstall.h"
#include "handler/temp_cleaner.h"
#include "handler/repair.h"

#include <cassert>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <stdexcept>
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
    assert(backup->contentHash != 0);
    {
        std::ofstream out(original, std::ios::trunc);
        out << "corrupted";
    }
    assert(restoreArtifact(*backup));
    std::ifstream restored(original);
    std::string restoredText;
    std::getline(restored, restoredText);
    assert(restoredText == "handler-batch-2");

    {
        std::ofstream out(backup->backup, std::ios::binary | std::ios::trunc);
        out << "handler-batch-!";
    }
    assert(!restoreArtifact(*backup));

    std::filesystem::remove_all(backupBase, testEc);

    const auto blockedBackupRoot = backupBase / "backup-root-file";
    {
        std::ofstream out(blockedBackupRoot);
        out << "not a directory";
    }
    const auto failedBackup = backupArtifact(original, blockedBackupRoot);
    assert(!failedBackup.has_value());

    ArtifactBackup missingBackup{original, backupBase / "missing.bak", true, 1, 1};
    assert(!restoreArtifact(missingBackup));

    const auto discovered = discoverComponents({"python", "definitely-not-a-handler-tool"});
    assert(discovered.size() <= 1);
    for (const auto& component : discovered) {
        assert(component.executable);
        assert(!component.path.empty());
        assert(std::filesystem::is_regular_file(component.path));
    }

    const auto trustRoot = std::filesystem::current_path() / ".handler-trust-boundary";
    std::filesystem::remove_all(trustRoot, testEc);
    std::filesystem::create_directories(trustRoot / "nested", testEc);
    assert(!testEc);
    const auto trustFile = trustRoot / "tool.exe";
    { std::ofstream out(trustFile); out << "fake"; }
    assert(pathUnder(trustFile, trustRoot));
    assert(pathUnder(trustRoot / "nested", trustRoot));
    assert(!pathUnder(trustRoot, trustRoot / "nested"));
    assert(!isTrustedExecutablePath(trustFile));
    std::filesystem::remove_all(trustRoot, testEc);

    const auto discoveryRoot = std::filesystem::current_path() / ".handler-discovery-boundary";
    std::filesystem::remove_all(discoveryRoot, testEc);
    std::filesystem::create_directories(discoveryRoot, testEc);
    assert(!testEc);
    const auto fakeTool = discoveryRoot / "handler-fake-tool";
    {
        std::ofstream out(fakeTool);
        out << "not an executable";
    }
#ifdef _WIN32
    const char* oldPathRaw = std::getenv("PATH");
    const std::string oldPath = oldPathRaw ? oldPathRaw : "";
    _putenv_s("PATH", discoveryRoot.string().c_str());
#else
    const char* oldPathRaw = std::getenv("PATH");
    const std::string oldPath = oldPathRaw ? oldPathRaw : "";
    setenv("PATH", discoveryRoot.string().c_str(), 1);
#endif
    const auto untrustedDiscovery = discoverComponents({"handler-fake-tool"});
#ifdef _WIN32
    _putenv_s("PATH", oldPath.c_str());
#else
    setenv("PATH", oldPath.c_str(), 1);
#endif
    assert(untrustedDiscovery.empty());
    std::filesystem::remove_all(discoveryRoot, testEc);

    const auto systemHealth = inspectSystem();
    assert(!systemHealth.pathAvailable || !systemHealth.pathValue.empty());
    assert(systemHealth.tempAvailable);

    assert(isAllowedExecutable("python"));
    assert(isAllowedExecutable("dotnet"));
    assert(isAllowedExecutable("npm"));
    assert(!isAllowedExecutable("format"));
    assert(isAllowedExecutable("winget"));
    assert(classifyCommandRisk("winget", {"upgrade", "--id", "Git.Git"}) == RiskLevel::High);

    const auto invalidExecutable = std::filesystem::temp_directory_path() / "handler-not-an-executable";
    CommandSpec invalidCommand{
        "invalid-explicit-path", "python", {"--version"}, RiskLevel::Low, 5000,
        {}, invalidExecutable};
    const auto invalidExecution = executeCommand(invalidCommand);
    assert(!invalidExecution.started);

    const auto lockTestFile = std::filesystem::temp_directory_path() / "handler-transaction-lock-test.lock";
    std::filesystem::remove(lockTestFile, testEc);
    {
        TransactionLock firstLock(lockTestFile);
        TransactionLock secondLock(lockTestFile);
        assert(firstLock.acquire());
        assert(firstLock.held());
        assert(!secondLock.acquire());
        firstLock.release();
        assert(!firstLock.held());
        assert(secondLock.acquire());
        assert(secondLock.held());
    }
    std::filesystem::remove(lockTestFile, testEc);

    const auto transactionA = RecoveryJournal::newTransactionId();
    const auto transactionB = RecoveryJournal::newTransactionId();
    assert(transactionA != transactionB);
    const auto journalIdentityFile = std::filesystem::temp_directory_path() / "handler-recovery-journal-identity-test.log";
    std::filesystem::remove(journalIdentityFile, testEc);
    RecoveryJournal identityJournal(journalIdentityFile);
    assert(identityJournal.record(transactionA, "START", "A"));
    assert(identityJournal.record(transactionB, "START", "B"));
    assert(identityJournal.record(transactionA, "COMMIT", "A"));
    assert(identityJournal.hasUnfinishedTransaction());
    const auto activeAfterA = identityJournal.unfinishedTransactionIds();
    assert(activeAfterA.size() == 1 && activeAfterA.front() == transactionB);
    assert(identityJournal.record(transactionB, "RECOVERY_REQUIRED", "B"));
    assert(identityJournal.hasUnfinishedTransaction());
    assert(identityJournal.record(transactionB, "MANUAL_ROLLBACK", "B"));
    assert(!identityJournal.hasUnfinishedTransaction());

    const auto failedSnapshotTransaction = RecoveryJournal::newTransactionId();
    assert(identityJournal.record(failedSnapshotTransaction, "START", "begin"));
    assert(identityJournal.record(failedSnapshotTransaction, "SNAPSHOT_FAILED", "snapshot unavailable"));
    assert(!identityJournal.hasCorruptEntries());
    const auto activeAfterSnapshotFailure = identityJournal.unfinishedTransactionIds();
    assert(activeAfterSnapshotFailure.size() == 1 &&
           activeAfterSnapshotFailure.front() == failedSnapshotTransaction);
    assert(identityJournal.record(failedSnapshotTransaction, "ABORT", "recovery snapshot unavailable"));
    assert(!identityJournal.hasUnfinishedTransaction());

    const auto multilineTransaction = RecoveryJournal::newTransactionId();
    assert(identityJournal.record(multilineTransaction, "START", "first line\nsecond line\rthird line"));
    assert(!identityJournal.hasCorruptEntries());
    const auto multilineActive = identityJournal.unfinishedTransactionIds();
    assert(multilineActive.size() == 1 && multilineActive.front() == multilineTransaction);
    assert(identityJournal.record(multilineTransaction, "COMMIT", "finished"));
    assert(!identityJournal.hasUnfinishedTransaction());

    // Transaction IDs are parsed as a single journal field and must not inject delimiters.
    assert(!identityJournal.record("invalid|tx=injected", "START", "must be rejected"));
    assert(!identityJournal.record("invalid\ntransaction", "START", "must be rejected"));
    assert(!identityJournal.hasCorruptEntries());
    assert(!identityJournal.hasUnfinishedTransaction());

    {
        std::ofstream legacy(journalIdentityFile, std::ios::app);
        legacy << "2026-10-07T00:00:00Z | START | legacy\n";
    }
    assert(identityJournal.hasUnfinishedTransaction());
    assert(identityJournal.unfinishedTransactionIds().size() == 1);
    assert(identityJournal.unfinishedTransactionIds().front().empty());
    assert(identityJournal.record("ROLLBACK", "legacy"));
    assert(!identityJournal.hasUnfinishedTransaction());
    {
        std::ofstream corrupt(journalIdentityFile, std::ios::app);
        corrupt << "corrupt journal record without delimiters\\n";
    }
    assert(identityJournal.hasCorruptEntries());
    assert(identityJournal.hasUnfinishedTransaction());
    std::filesystem::remove(journalIdentityFile, testEc);

    const auto journalBlockedRoot = std::filesystem::temp_directory_path() / "handler-journal-parent-file";
    std::filesystem::remove_all(journalBlockedRoot, testEc);
    { std::ofstream blocked(journalBlockedRoot); blocked << "not a directory"; }
    RecoveryJournal blockedJournal(journalBlockedRoot / "recovery.log");
    assert(!blockedJournal.record("START", "must fail"));
    std::filesystem::remove(journalBlockedRoot, testEc);

    // An existing but unreadable journal must fail closed, not look like an empty journal.
    const auto journalDirectoryPath = std::filesystem::temp_directory_path() / "handler-journal-directory";
    std::filesystem::remove_all(journalDirectoryPath, testEc);
    std::filesystem::create_directories(journalDirectoryPath, testEc);
    assert(!testEc);
    RecoveryJournal directoryJournal(journalDirectoryPath);
    assert(directoryJournal.hasCorruptEntries());
    assert(directoryJournal.hasUnfinishedTransaction());
    std::filesystem::remove_all(journalDirectoryPath, testEc);

    const auto journalTestFile = std::filesystem::temp_directory_path() / "handler-recovery-journal-test.log";
    std::filesystem::remove(journalTestFile, testEc);
    RecoveryJournal journalTest(journalTestFile);
    assert(journalTest.record("START", "test"));
    assert(journalTest.hasUnfinishedTransaction());
    assert(journalTest.record("ACTION_BEGIN", "test"));
    assert(journalTest.hasUnfinishedTransaction());
    assert(journalTest.record("COMMIT", "test"));
    assert(!journalTest.hasUnfinishedTransaction());
    assert(journalTest.record("START", "test"));
    assert(journalTest.record("MANUAL_ROLLBACK", "test"));
    assert(!journalTest.hasUnfinishedTransaction());
    assert(!journalTest.record("UNKNOWN_STAGE", "test"));
    {
        std::ofstream unknown(journalTestFile, std::ios::app);
        unknown << "2026-10-07T00:00:00Z | UNKNOWN_STAGE | injected\\n";
    }
    assert(journalTest.hasCorruptEntries());
    assert(journalTest.hasUnfinishedTransaction());
    assert(journalTest.record("START", "test"));
    assert(journalTest.record("ROLLBACK", "test"));
    // A terminal record must not make a journal with corrupt entries trusted again.
    assert(journalTest.hasCorruptEntries());
    assert(journalTest.hasUnfinishedTransaction());
    std::filesystem::remove(journalTestFile, testEc);

    bool rollbackCalled = false;
    Transaction failedAction(SafetyMode::Auto);
    const auto actionFailure = failedAction.run(
        RiskLevel::Low,
        [] { return false; },
        [] { return VerificationResult{true, "unused", "unused"}; },
        [&] { rollbackCalled = true; return true; });
    assert(!actionFailure.committed);
    assert(actionFailure.rolledBack);
    assert(rollbackCalled);

    rollbackCalled = false;
    Transaction failedVerification(SafetyMode::Auto);
    const auto verificationFailure = failedVerification.run(
        RiskLevel::Low,
        [] { return true; },
        [] { return VerificationResult{false, "integration test", "forced failure"}; },
        [&] { rollbackCalled = true; return true; });
    assert(!verificationFailure.committed);
    assert(verificationFailure.rolledBack);
    assert(rollbackCalled);

    rollbackCalled = false;
    Transaction throwingAction(SafetyMode::Auto);
    const auto actionException = throwingAction.run(
        RiskLevel::Low,
        []() -> bool { throw std::runtime_error("forced action exception"); },
        [] { return VerificationResult{true, "unused", "unused"}; },
        [&] { rollbackCalled = true; return true; });
    assert(!actionException.committed);
    assert(actionException.rolledBack);
    assert(rollbackCalled);

    rollbackCalled = false;
    Transaction throwingVerification(SafetyMode::Auto);
    const auto verificationException = throwingVerification.run(
        RiskLevel::Low,
        [] { return true; },
        []() -> VerificationResult { throw std::runtime_error("forced verification exception"); },
        [&] { rollbackCalled = true; return true; });
    assert(!verificationException.committed);
    assert(verificationException.rolledBack);
    assert(rollbackCalled);

    Transaction throwingRollback(SafetyMode::Auto);
    const auto rollbackException = throwingRollback.run(
        RiskLevel::Low,
        [] { return false; },
        [] { return VerificationResult{true, "unused", "unused"}; },
        []() -> bool { throw std::runtime_error("forced rollback exception"); });
    assert(!rollbackException.committed);
    assert(!rollbackException.rolledBack);
    assert(rollbackException.details == "action failed; rollback invoked");

    const auto cleanupRoot = std::filesystem::temp_directory_path() / "handler_cleanup_test";
    std::filesystem::remove_all(cleanupRoot, testEc);
    std::filesystem::create_directories(cleanupRoot, testEc);
    assert(!testEc);
    const auto oldFile = cleanupRoot / "old.txt";
    const auto recentFile = cleanupRoot / "recent.txt";
    { std::ofstream(oldFile) << "old"; }
    { std::ofstream(recentFile) << "recent"; }
    std::filesystem::last_write_time(
        oldFile, std::filesystem::file_time_type::clock::now() - std::chrono::hours(48), testEc);
    assert(!testEc);

    const auto dryRun = cleanTempDirectory(cleanupRoot, true);
    assert(dryRun.dryRun);
    assert(dryRun.candidates == 1);
    assert(std::filesystem::exists(oldFile));
    assert(std::filesystem::exists(recentFile));

    const auto cleaned = cleanTempDirectory(cleanupRoot, false);
    assert(cleaned.filesRemoved == 1);
    assert(!std::filesystem::exists(oldFile));
    assert(std::filesystem::exists(recentFile));
    std::filesystem::remove_all(cleanupRoot, testEc);

    Transaction rollbackTx(SafetyMode::Confirm);
    const auto rollbackOk = rollbackTx.runApproved(
        RiskLevel::Low,
        [] { return false; },
        [] { return VerificationResult{true, "not reached", ""}; },
        [] { return true; });
    assert(!rollbackOk.committed && rollbackOk.rolledBack);

    // runApproved must be one-shot: it must not silently approve the next high-risk run.
    bool highRiskActionRan = false;
    const auto highRiskAfterApproval = rollbackTx.run(
        RiskLevel::High,
        [&] { highRiskActionRan = true; return true; },
        [] { return VerificationResult{true, "verified", ""}; },
        [] { return true; });
    assert(!highRiskAfterApproval.committed);
    assert(!highRiskActionRan);

    // Explicit approval applies only to the transaction it approves, not nested runs.
    Transaction nestedApprovalTx(SafetyMode::Confirm);
    bool nestedHighRiskRan = false;
    bool nestedPolicyBlocked = false;
    const auto approvedOuter = nestedApprovalTx.runApproved(
        RiskLevel::Low,
        [&] {
            const auto nestedHighRisk = nestedApprovalTx.run(
                RiskLevel::High,
                [&] { nestedHighRiskRan = true; return true; },
                [] { return VerificationResult{true, "verified", ""}; },
                [] { return true; });
            nestedPolicyBlocked =
                nestedHighRisk.details == "user confirmation required before execution";
            return true;
        },
        [] { return VerificationResult{true, "outer verified", ""}; },
        [] { return true; });
    assert(approvedOuter.committed);
    assert(nestedPolicyBlocked);
    assert(!nestedHighRiskRan);

    Transaction rollbackFailTx(SafetyMode::Confirm);
    const auto rollbackFail = rollbackFailTx.runApproved(
        RiskLevel::Low,
        [] { return false; },
        [] { return VerificationResult{true, "not reached", ""}; },
        [] { return false; });
    assert(!rollbackFail.committed && !rollbackFail.rolledBack);

    // A failed rollback must remain visible as an unfinished recovery transaction.
    const auto isolatedStateRoot = std::filesystem::temp_directory_path() / "handler-rollback-recovery-state";
    std::filesystem::remove_all(isolatedStateRoot, testEc);
    const char* oldStateRootRaw = nullptr;
#ifdef _WIN32
    oldStateRootRaw = std::getenv("LOCALAPPDATA");
    const std::string oldStateRoot = oldStateRootRaw ? oldStateRootRaw : "";
    _putenv_s("LOCALAPPDATA", isolatedStateRoot.string().c_str());
#else
    oldStateRootRaw = std::getenv("XDG_STATE_HOME");
    const std::string oldStateRoot = oldStateRootRaw ? oldStateRootRaw : "";
    setenv("XDG_STATE_HOME", isolatedStateRoot.string().c_str(), 1);
#endif
    Transaction recoveryRequiredTx(SafetyMode::Auto);
    const auto recoveryRequired = recoveryRequiredTx.run(
        RiskLevel::Low,
        [] { return false; },
        [] { return VerificationResult{true, "unused", "unused"}; },
        [] { return false; });
    assert(!recoveryRequired.committed && !recoveryRequired.rolledBack);
    RecoveryJournal isolatedJournal(handlerTransactionRoot() / "recovery.log");
    assert(!isolatedJournal.hasCorruptEntries());
    assert(isolatedJournal.hasUnfinishedTransaction());
#ifdef _WIN32
    _putenv_s("LOCALAPPDATA", oldStateRoot.c_str());
#else
    if (oldStateRoot.empty()) unsetenv("XDG_STATE_HOME");
    else setenv("XDG_STATE_HOME", oldStateRoot.c_str(), 1);
#endif
    std::filesystem::remove_all(isolatedStateRoot, testEc);

    const auto low = evaluatePolicy(SafetyMode::Confirm, RiskLevel::Low);
    assert(low.allowed && !low.requiresConfirmation);

    const auto high = evaluatePolicy(SafetyMode::Confirm, RiskLevel::High);
    assert(!high.allowed && high.requiresConfirmation);

    const auto safeRisk = classifyCommandRisk("python", {"--version"});
    assert(safeRisk == RiskLevel::Low);
    const auto installRisk = classifyCommandRisk("npm", {"install", "express"});
    assert(installRisk == RiskLevel::High);

    std::error_code ec;
    const auto invalidDependencyRoot =
        std::filesystem::temp_directory_path() / "handler-invalid-dependency-test";
    std::filesystem::remove_all(invalidDependencyRoot, ec);

    // Inspection and version-selection edge cases must remain deterministic and side-effect free.
    const auto emptyDependencyRoot =
        std::filesystem::temp_directory_path() / "handler-empty-dependency-test";
    std::filesystem::remove_all(emptyDependencyRoot, ec);
    std::filesystem::create_directories(emptyDependencyRoot, ec);
    assert(!ec);
    const auto emptyPythonInfo = inspectDependencies(emptyDependencyRoot, "Python");
    assert(emptyPythonInfo.manifest.empty());
    assert(emptyPythonInfo.declared.empty());
    assert(!parseDependencyVersion("not-a-version").has_value());
    assert(!parseDependencyVersion("").has_value());
    assert(!selectCompatibleDependencyVersion(
        {">=4.0,<3.0"}, {"2.9.0", "3.5.0", "4.0.0"}).has_value());
    std::filesystem::remove_all(emptyDependencyRoot, ec);
    std::filesystem::create_directories(invalidDependencyRoot, ec);
    assert(upgradeDependency(invalidDependencyRoot, "Python", "bad;package", ">=1.0") == 3);
    assert(upgradeDependency(invalidDependencyRoot, "Node.js", "bad;package", ">=1.0") == 3);
    assert(upgradeDependency(invalidDependencyRoot, "Unknown", "package", ">=1.0") == 3);
    std::filesystem::remove_all(invalidDependencyRoot, ec);

    const auto fakeVirtualEnv =
        std::filesystem::temp_directory_path() / "handler-untrusted-virtualenv";
    std::filesystem::remove_all(fakeVirtualEnv, ec);
#ifdef _WIN32
    std::filesystem::create_directories(fakeVirtualEnv / "Scripts", ec);
    { std::ofstream out(fakeVirtualEnv / "Scripts" / "python.exe"); out << "fake"; }
    const char* oldVirtualEnvRaw = std::getenv("VIRTUAL_ENV");
    const std::string oldVirtualEnv = oldVirtualEnvRaw ? oldVirtualEnvRaw : "";
    _putenv_s("VIRTUAL_ENV", fakeVirtualEnv.string().c_str());
#else
    std::filesystem::create_directories(fakeVirtualEnv / "bin", ec);
    { std::ofstream out(fakeVirtualEnv / "bin" / "python"); out << "fake"; }
    const char* oldVirtualEnvRaw = std::getenv("VIRTUAL_ENV");
    const std::string oldVirtualEnv = oldVirtualEnvRaw ? oldVirtualEnvRaw : "";
    setenv("VIRTUAL_ENV", fakeVirtualEnv.string().c_str(), 1);
#endif
    assert(repairPythonModule("handler-test-package") == 3);
#ifdef _WIN32
    _putenv_s("VIRTUAL_ENV", oldVirtualEnv.c_str());
#else
    setenv("VIRTUAL_ENV", oldVirtualEnv.c_str(), 1);
#endif
    std::filesystem::remove_all(fakeVirtualEnv, ec);

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
    const auto tilde = selectCompatibleDependencyVersion(
        {"~1.4.0"}, {"1.3.9", "1.4.0", "1.4.9", "1.5.0"});
    assert(tilde.has_value() && *tilde == "1.4.9");
    const auto zeroCaret = selectCompatibleDependencyVersion(
        {"^0.2.0"}, {"0.1.9", "0.2.0", "0.2.8", "0.3.0"});
    assert(zeroCaret.has_value() && *zeroCaret == "0.2.8");
    const auto excluded = selectCompatibleDependencyVersion(
        {">=1.0,!=1.5.0,<2.0"}, {"1.4.9", "1.5.0", "1.8.0", "2.0.0"});
    assert(excluded.has_value() && *excluded == "1.8.0");
    assert(!satisfiesDependencyConstraint(*v250, "!=2.5.0"));

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
    std::filesystem::remove_all(tempRoot, ec);
    SnapshotStore snapshots(tempRoot);
    const auto capturedState = captureEnvironmentState();
    assert(capturedState.handlerVersion == "0.9.0");

    EnvironmentState state;
    state.timestampUtc = "test";
    state.computerName = "machine";
    state.userName = "user";
    state.handlerVersion = "0.9.0";
    state.currentDirectory = std::filesystem::current_path();
    const auto snapshot = snapshots.create(state);
    assert(snapshot.has_value());
    const auto secondSnapshot = snapshots.create(state);
    assert(secondSnapshot.has_value());
    assert(secondSnapshot->id != snapshot->id);
    assert(snapshots.find(snapshot->id).has_value());
    assert(snapshots.load(*snapshot).has_value());
    assert(snapshots.load(*secondSnapshot).has_value());
    assert(snapshots.list().size() == 2);
    auto malformedState = state;
    malformedState.userName = "user\ncomputer_name=injected";
    assert(!snapshots.create(malformedState).has_value());
    assert(snapshots.list().size() == 2);
    assert(!snapshots.find("../outside").has_value());
    SnapshotInfo traversal{"../outside", tempRoot / ".." / "outside.state"};
    assert(!snapshots.load(traversal).has_value());
    {
        std::ofstream corruptSnapshot(snapshot->path, std::ios::trunc);
        corruptSnapshot << "timestamp_utc=test\n"
                        << "computer_name=machine\n"
                        << "user_name=user\n"
                        << "temp_path=/tmp\n"
                        << "path=/usr/bin\n"
                        << "current_directory=/workspace\n"
                        << "unknown_field=value\n";
    }
    assert(!snapshots.load(*snapshot).has_value());
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
    assert(candidates[0].tool == "python");
    assert(candidates[0].supported);
    assert(candidates[1].tool == "definitely-not-a-handler-tool");
    assert(!candidates[1].supported);
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
