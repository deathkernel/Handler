#include "handler/policy.h"

namespace handler {

PolicyDecision evaluatePolicy(SafetyMode mode, RiskLevel risk) {
    if (mode == SafetyMode::Auto)
        return {risk == RiskLevel::Low, false, risk == RiskLevel::Low ? "low-risk action allowed" : "confirmation required for this risk"};

    if (mode == SafetyMode::Confirm)
        return {risk == RiskLevel::Low, risk != RiskLevel::Low, risk == RiskLevel::Low ? "low-risk action allowed" : "user confirmation required"};

    return {false, true, "strict mode requires explicit confirmation"};
}

std::string safetyModeName(SafetyMode mode) {
    switch (mode) {
    case SafetyMode::Auto: return "Auto";
    case SafetyMode::Confirm: return "Confirm";
    case SafetyMode::Strict: return "Strict";
    }
    return "Unknown";
}

} // namespace handler
