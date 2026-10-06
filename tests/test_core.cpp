#include "handler/command_engine.h"
#include "handler/dependency_graph.h"
#include "handler/error_detection.h"
#include "handler/policy.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace handler;

    assert(quoteArgument("plain") == "plain");
    assert(quoteArgument("hello world") == "\"hello world\"");
    assert(quoteArgument("a\\b c") == "\"a\\\\b c\"");

    assert(isAllowedExecutable("python"));
    assert(isAllowedExecutable("dotnet"));
    assert(isAllowedExecutable("npm"));
    assert(!isAllowedExecutable("format"));

    const auto low = evaluatePolicy(SafetyMode::Confirm, RiskLevel::Low);
    assert(low.allowed && !low.requiresConfirmation);

    const auto high = evaluatePolicy(SafetyMode::Confirm, RiskLevel::High);
    assert(!high.allowed && high.requiresConfirmation);

    const auto errors = detectErrors("ModuleNotFoundError: No module named 'requests'");
    assert(!errors.empty());

    const auto graph = buildDependencyGraph("demo", {"requests", "flask"});
    assert(graph.size() == 2);
    assert(graph[0].source == "demo");

    std::cout << "Handler core tests passed.\n";
    return 0;
}
