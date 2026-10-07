#include "handler/component_discovery.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace handler {

namespace {
bool trustedPath(const std::filesystem::path& path) {
#ifdef _WIN32
    std::error_code ec;
    const auto canonical = std::filesystem::weakly_canonical(path, ec);
    if (ec) return false;
    const auto current = std::filesystem::weakly_canonical(std::filesystem::current_path(), ec);
    if (!ec && canonical.parent_path() == current)
        return false;

    const char* pf = std::getenv("ProgramFiles");
    const char* pf86 = std::getenv("ProgramFiles(x86)");
    const char* la = std::getenv("LOCALAPPDATA");
    const char* win = std::getenv("WINDIR");
    const std::vector<std::filesystem::path> roots = {
        pf && *pf ? std::filesystem::path(pf) : std::filesystem::path{},
        pf86 && *pf86 ? std::filesystem::path(pf86) : std::filesystem::path{},
        la && *la ? std::filesystem::path(la) / "Programs" : std::filesystem::path{},
        la && *la ? std::filesystem::path(la) / "Microsoft" / "WindowsApps" : std::filesystem::path{},
        win && *win ? std::filesystem::path(win) / "System32" : std::filesystem::path{},
        win && *win ? std::filesystem::path(win) / "SysWOW64" : std::filesystem::path{}
    };
    for (const auto& root : roots) {
        if (root.empty()) continue;
        auto r = std::filesystem::weakly_canonical(root, ec);
        if (ec) { ec.clear(); continue; }
        auto a = r.begin();
        auto b = canonical.begin();
        bool same = true;
        for (; a != r.end() && b != canonical.end(); ++a, ++b) {
            if (_wcsicmp(a->wstring().c_str(), b->wstring().c_str()) != 0) { same = false; break; }
        }
        if (same && a == r.end()) return true;
    }
    return false;
#else
    return true;
#endif
}

std::string commandPath(const std::string& name) {
#ifdef _WIN32
    std::string candidate = name;
    if (candidate.find('.') == std::string::npos) candidate += ".exe";
    char buffer[MAX_PATH]{};
    const DWORD length = SearchPathA(nullptr, candidate.c_str(), nullptr, MAX_PATH, buffer, nullptr);
    if (!length) return {};
    return trustedPath(std::filesystem::path(std::string(buffer, length))) ? std::string(buffer, length) : std::string{};
#else
    const char* rawPath = std::getenv("PATH");
    if (!rawPath) return {};

    const std::string pathValue(rawPath);
    std::size_t begin = 0;
    while (begin <= pathValue.size()) {
        const auto end = pathValue.find(':', begin);
        const auto entry = pathValue.substr(
            begin, end == std::string::npos ? std::string::npos : end - begin);
        if (!entry.empty()) {
            const auto candidate = std::filesystem::path(entry) / name;
            std::error_code ec;
            if (std::filesystem::is_regular_file(candidate, ec) &&
                (ec.value() == 0)) {
                return std::filesystem::weakly_canonical(candidate, ec).string();
            }
        }
        if (end == std::string::npos) break;
        begin = end + 1;
    }
    return {};
#endif
}
}

std::vector<Component> discoverComponents(const std::vector<std::string>& names) {
    std::vector<Component> result;
    for (const auto& name : names) {
        const auto path = commandPath(name);
        if (!path.empty())
            result.push_back({name, "command", path, {}, true});
    }
    return result;
}

void printComponents(const std::vector<Component>& components) {
    if (components.empty()) {
        std::cout << "No requested components discovered.\n";
        return;
    }
    std::cout << "Discovered components:\n";
    for (const auto& component : components) {
        std::cout << "  " << component.name << " [" << component.kind << "] -> "
                  << component.path << '\n';
    }
}

} // namespace handler
