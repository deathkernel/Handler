#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace handler {
struct VariableFinding { std::string name; bool present{false}; std::string details; };
struct EnvironmentBaseline {
    std::vector<std::string> names;
};
std::vector<VariableFinding> inspectEnvironmentVariables(const std::vector<std::string>& names);
bool saveEnvironmentBaseline(const std::filesystem::path& file, const std::vector<std::string>& names);
bool loadEnvironmentBaseline(const std::filesystem::path& file, EnvironmentBaseline& baseline);
} 
