#pragma once
#include <string>
#include <vector>
namespace handler {
struct VariableFinding { std::string name; bool present{false}; std::string details; };
std::vector<VariableFinding> inspectEnvironmentVariables(const std::vector<std::string>& names);
}