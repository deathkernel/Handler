#include "handler/transaction.h"

namespace handler {

Transaction::Transaction(SafetyMode mode) : mode_(mode) {}

TransactionResult Transaction::run(RiskLevel risk, const Action& action,
                                   const Verify& verify, const Rollback& rollback) {
    const auto decision = evaluatePolicy(mode_, risk);
    if (!decision.allowed)
        return {false, false, decision.reason};

    if (!action || !verify) return {false, false, "invalid transaction callbacks"};
    if (!action()) {
        if (rollback) rollback();
        return {false, true, "action failed; rollback invoked"};
    }

    const auto result = verify();
    if (!result.passed) {
        if (rollback) rollback();
        return {false, true, "verification failed; rollback invoked"};
    }

    return {true, false, "action verified and committed"};
}

} // namespace handler
