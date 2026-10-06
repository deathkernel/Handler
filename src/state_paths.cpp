#include "handler/state_paths.h"

#include <cstdlib>

namespace handler {

std::filesystem::path handlerStateRoot() {
    if (const char* p = std::getenv("LOCALAPPDATA"); p && *p)
        return std::filesystem::path(p) / "Handler";
    if (const char* p = std::getenv("USERPROFILE"); p && *p)
        return std::filesystem::path(p) / ".handler";
    return std::filesystem::current_path() / ".handler";
}

std::filesystem::path handlerTransactionRoot() {
    return handlerStateRoot() / "transactions";
}

} // namespace handler
