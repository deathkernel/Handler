#include "handler/transaction.h"

#include "handler/environment_state.h"
#include "handler/state_paths.h"
#include "handler/recovery_journal.h"
#include "handler/transaction_lock.h"
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
    TransactionLock transactionLock(root / "transaction.lock");
    if (!transactionLock.acquire())
        return {false, false, false, {}, "transaction blocked: another Handler transaction is already active"};

    RecoveryJournal journal(root / "recovery.log");
    const auto transactionId = RecoveryJournal::newTransactionId();
    if (!journal.record(transactionId, "START", "transaction started"))
        return {false, false, false, {}, "transaction aborted: recovery journal unavailable"};

    SnapshotStore snapshots(root / "snapshots");
    const auto snapshot = snapshots.create(captureEnvironmentState());

    if (snapshot) {
        if (!journal.record(transactionId, "SNAPSHOT", snapshot->id))
            return {false, false, true, snapshot->id, "transaction aborted: recovery journal could not record snapshot"};
    } else {
        if (!journal.record(transactionId, "SNAPSHOT_FAILED", "environment snapshot could not be created"))
            return {false, false, false, {}, "transaction aborted: recovery journal unavailable"};
        if (risk != RiskLevel::Low) {
            journal.record(transactionId, "ABORT", "transaction refused because a recovery snapshot was unavailable");
            return {false, false, false, {}, "transaction aborted: recovery snapshot unavailable"};
        }
    }

    if (!journal.record(transactionId, "ACTION_BEGIN", "transaction action started"))
        return {false, false, snapshot.has_value(), snapshot ? snapshot->id : std::string{}, "transaction aborted: recovery journal unavailable before action"};

    const bool actionOk = action();
    if (!actionOk) {
        const bool rollbackOk = rollback ? rollback() : false;
        journal.record(transactionId, "ROLLBACK", rollbackOk ? "action failed; rollback verified by callback" : "action failed; rollback failed or unavailable");
        return {false, rollbackOk, snapshot.has_value(),
                snapshot ? snapshot->id : std::string{}, "action failed; rollback invoked"};
    }

    if (!journal.record(transactionId, "VERIFY_BEGIN", "transaction verification started")) {
        const bool rollbackOk = rollback ? rollback() : false;
        journal.record(transactionId, "ROLLBACK", rollbackOk ? "journal failure; rollback verified by callback" : "journal failure; rollback failed or unavailable");
        return {false, rollbackOk, snapshot.has_value(), snapshot ? snapshot->id : std::string{}, "verification aborted: recovery journal unavailable"};
    }

    const auto verification = verify();
    if (!verification.passed) {
        const bool rollbackOk = rollback ? rollback() : false;
        journal.record(transactionId, "ROLLBACK", rollbackOk ? "verification failed; rollback verified by callback" : "verification failed; rollback failed or unavailable");
        return {false, rollbackOk, snapshot.has_value(),
                snapshot ? snapshot->id : std::string{},
                "verification failed; rollback invoked"};
    }

    const bool commitRecorded = journal.record(transactionId, "COMMIT", "transaction verified");
    return {true, false, snapshot.has_value(),
            snapshot ? snapshot->id : std::string{},
            commitRecorded ? "action verified and committed"
                           : "action verified and committed; recovery journal persistence failed"};
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
