#pragma once
#include <string>
namespace handler {
enum class CommandRisk { Safe, Review, Blocked };
struct CommandRiskResult { CommandRisk risk{CommandRisk::Safe}; std::string reason; };
CommandRiskResult inspectCommandRisk(const std::string& commandLine);
}