#include "handler/tool_updates.h"
#include "handler/component_discovery.h"
namespace handler {
std::vector<UpdateCandidate> inspectToolUpdates(const std::vector<std::string>& tools) {
    std::vector<UpdateCandidate> out;
    for (const auto& c : discoverComponents(tools))
        out.push_back({c.name, c.path, c.path.empty() ? "install/update source should be reviewed" : "update source should be reviewed"});
    return out;
}
}