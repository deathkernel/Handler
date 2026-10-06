#include "handler/state_paths.h"

#include <cstdlib>
#include <filesystem>

namespace handler {

std::filesystem::path handlerStateRoot() {
#ifdef _WIN32
    if (const char* p = std::getenv("LOCALAPPDATA"); p && *p)
        return std::filesystem::path(p) / "Handler";
    if (const char* p = std::getenv("USERPROFILE"); p && *p)
        return std::filesystem::path(p) / ".handler";
#else
    if (const char* p = std::getenv("XDG_STATE_HOME"); p && *p)
        return std::filesystem::path(p) / "handler";
    if (const char* p = std::getenv("HOME"); p && *p)
        return std::filesystem::path(p) / ".local" / "state" / "handler";
#endif
    return std::filesystem::current_path() / ".handler";
}

std::filesystem::path handlerTransactionRoot() {
    return handlerStateRoot() / "transactions";
}

} // namespace handler
