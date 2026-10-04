#pragma once
#include <filesystem>
#include <string>
namespace handler {
struct UninstallPlan { bool allowed{false}; std::filesystem::path target; std::string reason; };
UninstallPlan planUninstall(const std::filesystem::path& target);
}