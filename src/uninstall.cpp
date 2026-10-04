#include "handler/uninstall.h"
namespace handler {
UninstallPlan planUninstall(const std::filesystem::path& target) {
    if (target.empty()) return {false, {}, "empty target"};
    std::error_code ec;
    const auto p = std::filesystem::weakly_canonical(target, ec);
    if (ec) return {false, target, "target cannot be resolved"};
    if (p == p.root_path()) return {false, p, "root paths are never uninstallable"};
    return {true, p, "target resolved; explicit confirmation still required"};
}
}