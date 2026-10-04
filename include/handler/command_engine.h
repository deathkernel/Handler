#pragma once

#include "handler/policy.h"

#include <string>
#include <vector>

namespace handler {

struct CommandSpec {
    std::string id;
    std::string executable;
    std::vector<std::string> arguments;
    RiskLevel risk{RiskLevel::Medium};
};

std::string quoteArgument(const std::string& value);
std::string buildCommandLine(const CommandSpec& command);
bool isAllowedExecutable(const std::string& executable);

} // namespace handler
