#include "handler/dependency_graph.h"

#include <iostream>
#include <set>\n#include <algorithm>\n#include <utility>

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

std::vector<DependencyEdge> buildTransitiveDependencyGraph(
    const std::vector<DependencyEdge>& directEdges,
    const std::vector<DependencyEdge>& transitiveEdges) {
    std::vector<DependencyEdge> out = directEdges;
    for (const auto& edge : transitiveEdges) {
        auto it = std::find_if(out.begin(), out.end(), [&](const DependencyEdge& existing) {
            return existing.source == edge.source && existing.target == edge.target &&
                   existing.relation == edge.relation;
        });
        if (it == out.end()) out.push_back(edge);
    }
    return out;
}

std::vector<DependencyImpact> analyzeDependencyImpact(
    const std::vector<DependencyEdge>& edges,
    const std::string& dependency) {
    std::vector<DependencyImpact> out;
    std::set<std::string> affected;
    for (const auto& edge : edges) {
        if (edge.target == dependency) affected.insert(edge.source);
    }
    DependencyImpact impact;
    impact.dependency = dependency;
    impact.affected.assign(affected.begin(), affected.end());
    impact.risk = impact.affected.size() > 1 ? "HIGH" :
                  impact.affected.size() == 1 ? "MEDIUM" : "LOW";
    out.push_back(std::move(impact));
    return out;
}

void printDependencyImpact(const std::vector<DependencyImpact>& impacts) {
    std::cout << "Dependency impact:\n";
    for (const auto& impact : impacts) {
        std::cout << "  " << impact.dependency << " | risk=" << impact.risk;
        if (impact.affected.empty()) std::cout << " | no dependents discovered";
        else {
            std::cout << " | affected: ";
            for (std::size_t i = 0; i < impact.affected.size(); ++i)
                std::cout << (i ? ", " : "") << impact.affected[i];
        }
        std::cout << "\n";
    }
}
