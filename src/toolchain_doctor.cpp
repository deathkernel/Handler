#include "handler/toolchain_doctor.h"
#include "handler/component_discovery.h"
namespace handler {
std::vector<ToolchainFinding> inspectToolchain(const std::vector<std::string>& tools) {
    std::vector<ToolchainFinding> out;
    for (const auto& c : discoverComponents(tools))
        out.push_back({c.name, !c.path.empty(), c.path.empty() ? "not found" : c.path});
    return out;
}
}