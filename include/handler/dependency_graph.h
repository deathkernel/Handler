#pragma once

#include <string>
#include <vector>

namespace handler {

struct DependencyEdge {
    std::string source;
    std::string target;
    std::string relation;
};

std::vector<DependencyEdge> buildDependencyGraph(const std::string& project,
                                                 const std::vector<std::string>& dependencies);
void printDependencyGraph(const std::vector<DependencyEdge>& edges);

} // namespace handler
