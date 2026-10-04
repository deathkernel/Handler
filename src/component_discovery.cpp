#include "handler/component_discovery.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace handler {

namespace {
std::string commandPath(const std::string& name) {
#ifdef _WIN32
    std::string candidate = name;
    if (candidate.find('.') == std::string::npos) candidate += ".exe";
    char buffer[MAX_PATH]{};
    const DWORD length = SearchPathA(nullptr, candidate.c_str(), nullptr, MAX_PATH, buffer, nullptr);
    return length ? std::string(buffer, length) : std::string{};
#else
    return {};
#endif
}
}

std::vector<Component> discoverComponents(const std::vector<std::string>& names) {
    std::vector<Component> result;
    for (const auto& name : names) {
        const auto path = commandPath(name);
        if (!path.empty()) {
            result.push_back({name, "command", path, {}, true});
        }
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
