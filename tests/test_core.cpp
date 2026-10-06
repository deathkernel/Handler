#include "handler/command_engine.h"
#include "handler/dependency_graph.h"
#include "handler/error_detection.h"
#include "handler/policy.h"
#include "handler/snapshot.h"

#include <cassert>
#include <filesystem>
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

    const auto tempRoot = std::filesystem::temp_directory_path() / "handler-core-test";
    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);

    SnapshotStore snapshots(tempRoot);
    EnvironmentState state;
    state.timestampUtc = "2026-10-06T00:00:00Z";
    state.computerName = "test-machine";
    state.userName = "test-user";
    state.tempPath = "C:/Temp";
    state.pathValue = "C:/Tools";
    state.currentDirectory = "C:/Project";
    state.handlerVersion = "0.7.0";

    const auto snapshot = snapshots.create(state);
    assert(snapshot.has_value());

    const auto loaded = snapshots.load(*snapshot);
    assert(loaded.has_value());
    assert(loaded->computerName == state.computerName);
    assert(loaded->pathValue == state.pathValue);
    assert(loaded->currentDirectory == state.currentDirectory);

    std::filesystem::remove_all(tempRoot, ec);

    std::cout << "Handler core tests passed.\n";
    return 0;
}
