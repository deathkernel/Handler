#pragma once
#include <string>
#include <vector>
namespace handler {
struct PathFinding { std::string entry; bool exists{false}; std::string details; };
std::vector<PathFinding> inspectPathEntries();
}