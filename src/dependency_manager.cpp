#include "handler/dependency_manager.h"

#include <fstream>
#include <iostream>
#include <regex>
#include <string>

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
