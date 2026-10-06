#include "handler/transaction.h"

#include "handler/environment_state.h"
#include "handler/recovery_journal.h"
#include <cstdlib>

#include <filesystem>

namespace handler {

Transaction::Transaction(SafetyMode mode) : mode_(mode) {}

TransactionResult Transaction::run(RiskLevel risk, const Action& action,
                                   const Verify& verify, const Rollback& rollback) {
    const auto decision = evaluatePolicy(mode_, risk);
    if (!decision.allowed)
        return {false, false, false, {}, decision.reason};

    if (!action || !verify)
        return {false, false, false, {}, "invalid transaction callbacks"};

    std::filesystem::path root;
    if (const char* p = std::getenv("LOCALAPPDATA"); p && *p)
        root = std::filesystem::path(p) / "Handler" / "transactions";
    else if (const char* p = std::getenv("USERPROFILE"); p && *p)
        root = std::filesystem::path(p) / ".handler" / "transactions";
    else
        root = std::filesystem::current_path() / ".handler" / "transactions";
    RecoveryJournal journal(root / "recovery.log");
    journal.record("START", "transaction started");

    SnapshotStore snapshots(root / "snapshots");
    const auto snapshot = snapshots.create(captureEnvironmentState());

    if (snapshot) {
        journal.record("SNAPSHOT", snapshot->id);
    } else {
        journal.record("SNAPSHOT_FAILED", "environment snapshot could not be created");
        if (risk != RiskLevel::Low) {
            journal.record("ABORT", "transaction refused because a recovery snapshot was unavailable");
            return {false, false, false, {}, "transaction aborted: recovery snapshot unavailable"};
        }
    }

    const bool actionOk = action();
    if (!actionOk) {
        if (rollback) rollback();
        journal.record("ROLLBACK", "action failed");
        return {false, static_cast<bool>(rollback), snapshot.has_value(),
                snapshot ? snapshot->id : std::string{}, "action failed; rollback invoked"};
    }

    const auto verification = verify();
    if (!verification.passed) {
        if (rollback) rollback();
        journal.record("ROLLBACK", "verification failed: " + verification.details);
        return {false, static_cast<bool>(rollback), snapshot.has_value(),
                snapshot ? snapshot->id : std::string{},
                "verification failed; rollback invoked"};
    }

    journal.record("COMMIT", "transaction verified");
    return {true, false, snapshot.has_value(),
            snapshot ? snapshot->id : std::string{},
            "action verified and committed"};
}


TransactionResult Transaction::runApproved(RiskLevel risk, const Action& action,
                                           const Verify& verify, const Rollback& rollback) {
    if (!action || !verify)
        return {false, false, false, {}, "invalid transaction callbacks"};
    const auto previous = mode_;
    mode_ = SafetyMode::Auto;
    const auto result = [&] {
        const auto decision = evaluatePolicy(SafetyMode::Confirm, RiskLevel::Low);
        if (!decision.allowed) return TransactionResult{false, false, false, {}, "pre-confirmed transaction rejected"};
        return run(RiskLevel::Low, action, verify, rollback);
    }();
    mode_ = previous;
    return result;
}

} // namespace handler
