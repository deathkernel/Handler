#include "handler/transaction.h"

#include "handler/environment_state.h"
#include "handler/recovery_journal.h"

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

    const std::filesystem::path root =
        std::filesystem::current_path() / ".handler" / "transactions";
    RecoveryJournal journal(root / "recovery.log");
    journal.record("START", "transaction started");

    SnapshotStore snapshots(root / "snapshots");
    const auto snapshot = snapshots.create(captureEnvironmentState());

    if (snapshot) {
        journal.record("SNAPSHOT", snapshot->id);
    } else {
        journal.record("SNAPSHOT_FAILED", "environment snapshot could not be created");
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

} // namespace handler
