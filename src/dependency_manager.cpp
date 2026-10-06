#include "handler/dependency_manager.h"

#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <set>
#include <sstream>

namespace handler {

namespace {
void collectSimpleLines(const std::filesystem::path& file,
                        std::vector<std::string>& out) {
    std::ifstream input(file);
    if (!input) return;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') continue;
        out.push_back(line);
    }
}
}

DependencyInfo inspectDependencies(const std::filesystem::path& root,
                                   const std::string& type) {
    DependencyInfo info;
    info.ecosystem = type;

    if (type == "Python" && std::filesystem::exists(root / "requirements.txt")) {
        info.manifest = "requirements.txt";
        collectSimpleLines(root / "requirements.txt", info.declared);
    } else if (type == "Node.js" &&
               std::filesystem::exists(root / "package.json")) {
        info.manifest = "package.json";
        std::ifstream input(root / "package.json");
        std::string line;
        const std::regex dependencyLine(
            R"DELIM(^\s*"([^"]+)"\s*:\s*"([^"]+)")DELIM");
        while (std::getline(input, line)) {
            std::smatch match;
            if (std::regex_search(line, match, dependencyLine))
                info.declared.push_back(match[1].str() + " " + match[2].str());
        }
    } else if (type == "Rust" &&
               std::filesystem::exists(root / "Cargo.toml")) {
        info.manifest = "Cargo.toml";
        collectSimpleLines(root / "Cargo.toml", info.declared);
    } else if (type == "Go" && std::filesystem::exists(root / "go.mod")) {
        info.manifest = "go.mod";
        collectSimpleLines(root / "go.mod", info.declared);
    } else if (type == "C/C++" &&
               std::filesystem::exists(root / "CMakeLists.txt")) {
        info.manifest = "CMakeLists.txt";
    }
    return info;
}

void printDependencies(const DependencyInfo& info) {
    if (info.manifest.empty()) {
        std::cout << "Dependency inspection: no supported manifest found.\n";
        return;
    }
    std::cout << "Dependency inspection [" << info.ecosystem
              << "] via " << info.manifest << ":\n";
    for (const auto& dep : info.declared)
        std::cout << "  " << dep << '\n';
    if (info.declared.empty())
        std::cout << "  No dependency entries parsed by the current lightweight scanner.\n";
}

} // namespace handler

std::vector<DependencyRequirement> parseDependencyRequirements(const DependencyInfo& info) {
    std::vector<DependencyRequirement> out;
    for (const auto& raw : info.declared) {
        std::string line = raw;
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.erase(line.begin());
        if (line.empty() || line[0] == '#' || line[0] == '[') continue;
        const auto pos = line.find_first_of("=<>!~ ");
        const std::string name = pos == std::string::npos ? line : line.substr(0, pos);
        std::string constraint = pos == std::string::npos ? "*" : line.substr(pos);
        if (!name.empty()) out.push_back({name, constraint, true});
    }
    return out;
}

std::vector<DependencyConflict> findDependencyConflicts(
    const std::vector<DependencyRequirement>& requirements) {
    std::unordered_map<std::string, std::vector<std::string>> grouped;
    for (const auto& r : requirements) grouped[r.name].push_back(r.constraint);
    std::vector<DependencyConflict> out;
    for (const auto& [name, constraints] : grouped) {
        if (constraints.size() > 1 && !dependencyConstraintsCompatible(constraints))
            out.push_back({name, constraints.front(), constraints.back(),
                           "no version satisfies the combined constraints"});
    }
    return out;
}

std::vector<DependencyCandidate> proposeDependencyUpgrades(
    const std::vector<DependencyRequirement>& requirements) {
    std::vector<DependencyCandidate> out;
    for (const auto& r : requirements) {
        const bool compatible = dependencyConstraintsCompatible({r.constraint});
        out.push_back({r.name, "unknown", r.constraint,
                       compatible ? "REVIEW" : "BLOCKED",
                       compatible ? "registry lookup required before changing the manifest"
                                  : "constraint is internally unsatisfiable", {}, {}});
    }
    return out;
}

void printDependencyAnalysis(const std::vector<DependencyConflict>& conflicts,
                             const std::vector<DependencyCandidate>& candidates) {
    std::cout << "Dependency analysis:\n";
    std::cout << "  Conflicts: " << conflicts.size() << "\n";
    for (const auto& c : conflicts)
        std::cout << "    [CONFLICT] " << c.name << ": " << c.left
                  << " vs " << c.right << " | " << c.reason << "\n";
    std::cout << "  Upgrade candidates: " << candidates.size() << "\n";
    for (const auto& c : candidates)
        std::cout << "    [" << c.action << "] " << c.name
                  << " " << c.constraint << " | " << c.reason << "\n";
}
