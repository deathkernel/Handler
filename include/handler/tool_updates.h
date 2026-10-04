#pragma once
#include <string>
#include <vector>
namespace handler {
struct UpdateCandidate { std::string tool; std::string currentPath; std::string updateHint; };
std::vector<UpdateCandidate> inspectToolUpdates(const std::vector<std::string>& tools);
}