#include "handler/toolchain_doctor.h"
#include "handler/component_discovery.h"

#include <unordered_map>

namespace handler {

std::vector<ToolchainFinding> inspectToolchain(
    const std::vector<std::string>& tools) {
    std::vector<ToolchainFinding> out;
    const auto discovered = discoverComponents(tools);

    std::unordered_map<std::string, Component> byName;
    for (const auto& component : discovered)
        byName[component.name] = component;

    for (const auto& name : tools) {
        const auto it = byName.find(name);
        if (it == byName.end())
            out.push_back({name, false, "not found"});
        else
            out.push_back({name, true, it->second.path});
    }
    return out;
}

} // namespace handler
