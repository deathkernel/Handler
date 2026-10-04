#include "handler/environment_guardian.h"
#include <cstdlib>
namespace handler {
std::vector<VariableFinding> inspectEnvironmentVariables(const std::vector<std::string>& names) {
    std::vector<VariableFinding> out;
    for (const auto& name : names) {
        const char* value = std::getenv(name.c_str());
        out.push_back({name, value && *value, value && *value ? "set" : "not set"});
    }
    return out;
}
}