#include "handler/path_guardian.h"
#include <cstdlib>
#include <filesystem>
namespace handler {
std::vector<PathFinding> inspectPathEntries() {
    std::vector<PathFinding> out;
    const char* raw = std::getenv("PATH");
    if (!raw) return out;
    std::string path(raw);
    size_t start = 0;
    while (start <= path.size()) {
        const size_t end = path.find(';', start);
        const std::string entry = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!entry.empty()) {
            std::error_code ec;
            const bool exists = std::filesystem::is_directory(entry, ec);
            out.push_back({entry, exists, exists ? "directory exists" : (ec ? ec.message() : "directory missing")});
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return out;
}
}