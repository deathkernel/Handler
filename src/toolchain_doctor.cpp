#include "handler/toolchain_doctor.h"
#include "handler/action_engine.h"
#include "handler/component_discovery.h"
#include "handler/transaction.h"
#include "handler/verification.h"
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace handler {
namespace {
std::string versionCommand(const std::string& tool) {
    if (tool == "npm") return "npm --version";
    return tool + " --version";
}
std::string firstLine(const std::string& text) {
    const auto p = text.find_first_of("\r\n");
    return text.substr(0, p == std::string::npos ? text.size() : p);
}
}
std::vector<ToolchainFinding> inspectToolchain(const std::vector<std::string>& tools) {
    std::vector<ToolchainFinding> out;
    const auto discovered = discoverComponents(tools);
    std::unordered_map<std::string, Component> byName;
    for (const auto& component : discovered) byName[component.name] = component;

    for (const auto& name : tools) {
        const auto it = byName.find(name);
        if (it == byName.end()) {
            out.push_back({name, false, {}, {}, "MISSING", "not found"});
            continue;
        }
        CommandSpec spec{"toolchain-version", name, {"--version"}, RiskLevel::Low, 15000};
        if (name == "npm") spec.arguments = {"--version"};
        const auto result = executeCommand(spec);
        const std::string version = result.started && result.exitCode == 0
            ? firstLine(result.output) : "version unavailable";
        const bool healthy = result.started && result.exitCode == 0;
        out.push_back({name, true, it->second.path, version,
                       healthy ? "HEALTHY" : "DEGRADED",
                       healthy ? "version check passed" : "version command failed"});
    }
    return out;
}
std::vector<ToolchainRepair> proposeToolchainRepairs(
    const std::vector<ToolchainFinding>& findings) {
    std::vector<ToolchainRepair> out;
    for (const auto& f : findings) {
        if (f.status == "HEALTHY") continue;
        if (f.tool == "python" || f.tool == "node" || f.tool == "git" ||
            f.tool == "cmake" || f.tool == "dotnet")
            out.push_back({f.tool, true,
                           f.tool == "python" ? "python --version" :
                           f.tool + " --version",
                           f.status == "MISSING" ? "tool missing; installation source must be reviewed"
                                                  : "tool version check failed"});
        else
            out.push_back({f.tool, false, {}, "automatic repair source is not defined"});
    }
    return out;
}
}

bool repairToolchain(const std::string& tool, std::string& details) {
    // Only package-manager-owned, in-place upgrades with a deterministic
    // package manager are eligible here. Missing runtimes are never installed
    // automatically because their installer/source must be explicitly chosen.
    std::string executable;
    std::vector<std::string> args;
    if (tool == "python") {
#ifdef _WIN32
        executable = "python";
        args = {"-m", "pip", "install", "--upgrade", "pip"};
#else
        executable = "python";
        args = {"-m", "pip", "install", "--upgrade", "pip"};
#endif
    } else if (tool == "node") {
        details = "Node runtime repair requires an explicit installer/source; automatic repair is blocked";
        return false;
    } else if (tool == "git" || tool == "cmake" || tool == "dotnet") {
        details = "runtime/tool repair requires an explicit installer/source; automatic repair is blocked";
        return false;
    } else {
        details = "unsupported toolchain repair target";
        return false;
    }

    Transaction tx(SafetyMode::Confirm);
    const auto result = tx.run(
        RiskLevel::High,
        [&] {
            CommandSpec action{"toolchain-repair", executable, args, RiskLevel::High, 180000};
            const auto r = executeCommand(action);
            details = r.output.empty() ? r.error : r.output;
            return r.started && r.exitCode == 0;
        },
        [&] {
            const auto findings = inspectToolchain({tool});
            const auto it = findings.empty() ? findings.end() : findings.begin();
            const bool ok = it != findings.end() && it->available &&
                            it->status == "HEALTHY";
            return VerificationResult{ok, "toolchain health check",
                                      ok ? it->version : "post-repair health check failed"};
        },
        [&] {
            details += " | rollback: no package-level downgrade is attempted; transaction snapshot retained";
        });
    if (!result.committed) {
        details += " | transaction=" + result.details +
                   " | snapshot=" + result.snapshotId;
        return false;
    }
    details = "verified toolchain repair committed | snapshot=" + result.snapshotId;
    return true;
}
