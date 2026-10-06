#include "handler/transaction.h"

#include "handler/environment_state.h"
#include "handler/state_paths.h"
#include "handler/recovery_journal.h"
#include <cstdlib>

#include <filesystem>

namespace handler {

Transaction::Transaction(SafetyMode mode) : mode_(mode) {}

TransactionResult Transaction::run(RiskLevel risk, const Action& action,
                                   const Verify& verify, const Rollback& rollback) {
    const auto decision = preApproved_ ? PolicyDecision{true, false, "explicitly pre-confirmed"} : evaluatePolicy(mode_, risk);
    if (!decision.allowed)
        return {false, false, false, {}, decision.reason};

    if (!action || !verify)
        return {false, false, false, {}, "invalid transaction callbacks"};

    const auto root = handlerTransactionRoot();
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
        const bool rollbackOk = rollback ? rollback() : false;
        journal.record("ROLLBACK", rollbackOk ? "action failed; rollback verified by callback" : "action failed; rollback failed or unavailable");
        return {false, rollbackOk, snapshot.has_value(),
                snapshot ? snapshot->id : std::string{}, "action failed; rollback invoked"};
    }

    const auto verification = verify();
    if (!verification.passed) {
        const bool rollbackOk = rollback ? rollback() : false;
        journal.record("ROLLBACK", rollbackOk ? "verification failed; rollback verified by callback" : "verification failed; rollback failed or unavailable");
        return {false, rollbackOk, snapshot.has_value(),
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
    preApproved_ = true;
    const auto result = run(risk, action, verify, rollback);
    preApproved_ = false;
    return result;
}

} // namespace handler
