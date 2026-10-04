#pragma once
#include <string>
#include <vector>
namespace handler {
struct ToolchainFinding { std::string tool; bool available{false}; std::string details; };
std::vector<ToolchainFinding> inspectToolchain(const std::vector<std::string>& tools);
}