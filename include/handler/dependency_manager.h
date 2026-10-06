#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace handler {
struct DependencyInfo {
    std::string ecosystem;
    std::string manifest;
    std::vector<std::string> declared;
};
struct DependencyRequirement {
    std::string name;
    std::string constraint;
    bool direct{true};
};
struct DependencyConflict {
    std::string name;
    std::string left;
    std::string right;
    std::string reason;
};
struct DependencyCandidate {
    std::string name;
    std::string current;
    std::string constraint;
    std::string action;
    std::string reason;
};
DependencyInfo inspectDependencies(const std::filesystem::path& projectRoot,
                                   const std::string& projectType);
std::vector<DependencyRequirement> parseDependencyRequirements(const DependencyInfo& info);
std::vector<DependencyConflict> findDependencyConflicts(
    const std::vector<DependencyRequirement>& requirements);
std::vector<DependencyCandidate> proposeDependencyUpgrades(
    const std::vector<DependencyRequirement>& requirements);
void printDependencies(const DependencyInfo& info);
void printDependencyAnalysis(const std::vector<DependencyConflict>& conflicts,
                             const std::vector<DependencyCandidate>& candidates);
} // namespace handler
