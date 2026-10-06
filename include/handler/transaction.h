#pragma once

#include "handler/policy.h"
#include "handler/snapshot.h"
#include "handler/verification.h"

#include <functional>
#include <string>

namespace handler {

struct TransactionResult {
    bool committed{false};
    bool rolledBack{false};
    bool snapshotCreated{false};
    std::string snapshotId;
    std::string details;
};

class Transaction {
public:
    using Action = std::function<bool()>;
    using Rollback = std::function<void()>;
    using Verify = std::function<VerificationResult()>;

    explicit Transaction(SafetyMode mode);
    TransactionResult run(RiskLevel risk, const Action& action,
                          const Verify& verify, const Rollback& rollback);
    TransactionResult runApproved(RiskLevel risk, const Action& action,
                                  const Verify& verify, const Rollback& rollback);

private:
    SafetyMode mode_;
};

} // namespace handler
