#include "handler/environment_guardian.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
namespace handler {
std::vector<VariableFinding> inspectEnvironmentVariables(const std::vector<std::string>& names) {
    std::vector<VariableFinding> out;
    for (const auto& name : names) {
        const char* value = std::getenv(name.c_str());
        out.push_back({name, value && *value, value && *value ? "set" : "not set"});
    }
    return out;
}
bool saveEnvironmentBaseline(const std::filesystem::path& file, const std::vector<std::string>& names) {
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);
    if (ec) return false;
    std::ofstream out(file);
    if (!out) return false;
    // Store names only. Values may contain secrets and are deliberately not persisted.
    for (const auto& name : names) out << name << '\n';
    return out.good();
}
bool loadEnvironmentBaseline(const std::filesystem::path& file, EnvironmentBaseline& baseline) {
    std::ifstream in(file);
    if (!in) return false;
    baseline.names.clear();
    std::string name;
    while (std::getline(in, name))
        if (!name.empty()) baseline.names.push_back(name);
    return true;
}
}
