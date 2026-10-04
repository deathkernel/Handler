#include "handler/dependency_graph.h"

#include <iostream>

namespace handler {

std::vector<DependencyEdge> buildDependencyGraph(const std::string& project, const std::vector<std::string>& dependencies) {
    std::vector<DependencyEdge> graph;
    for (const auto& dependency : dependencies) graph.push_back({project, dependency, "declares"});
    return graph;
}

void printDependencyGraph(const std::vector<DependencyEdge>& edges) {
    std::cout << "Dependency graph:\n";
    for (const auto& edge : edges) std::cout << "  " << edge.source << " --" << edge.relation << "--> " << edge.target << '\\n';
    if (edges.empty()) std::cout << "  No edges discovered.\n";
}

} // namespace handler
