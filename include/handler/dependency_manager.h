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

DependencyInfo inspectDependencies(const std::filesystem::path& projectRoot,
                                   const std::string& projectType);
void printDependencies(const DependencyInfo& info);

} // namespace handler
