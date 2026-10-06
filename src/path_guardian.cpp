#include "handler/path_guardian.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace handler {
namespace {
char separator() {
#ifdef _WIN32
    return ';';
#else
    return ':';
#endif
}
std::vector<std::string> currentEntries() {
    std::vector<std::string> out;
    const char* raw = std::getenv("PATH");
    if (!raw) return out;
    std::stringstream ss(raw);
    std::string item;
    while (std::getline(ss, item, separator()))
        if (!item.empty()) out.push_back(item);
    return out;
}
}
std::vector<PathFinding> inspectPathEntries() {
    std::vector<PathFinding> out;
    for (const auto& entry : currentEntries()) {
        std::error_code ec;
        const bool exists = std::filesystem::is_directory(entry, ec);
        out.push_back({entry, exists, exists ? "directory exists" :
                       (ec ? ec.message() : "directory missing")});
    }
    return out;
}
PathRepairPlan analyzePathEntries() {
    PathRepairPlan plan;
    std::unordered_set<std::string> seen;
    for (const auto& entry : currentEntries()) {
        if (!seen.insert(entry).second) plan.duplicateEntries.push_back(entry);
        std::error_code ec;
        if (!std::filesystem::is_directory(entry, ec))
            plan.missingEntries.push_back(entry);
    }
    plan.details = "diagnostic only; no OS PATH changes were applied";
    return plan;
}
bool savePathBaseline(const std::filesystem::path& file) {
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);
    if (ec) return false;
    std::ofstream out(file);
    if (!out) return false;
    for (const auto& entry : currentEntries()) out << entry << '\n';
    return out.good();
}
bool loadPathBaseline(const std::filesystem::path& file, std::vector<std::string>& entries) {
    std::ifstream in(file);
    if (!in) return false;
    entries.clear();
    std::string line;
    while (std::getline(in, line))
        if (!line.empty()) entries.push_back(line);
    return true;
}
} // namespace handler
