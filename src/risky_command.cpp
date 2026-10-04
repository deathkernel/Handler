#include "handler/risky_command.h"
#include <algorithm>
namespace handler {
CommandRiskResult inspectCommandRisk(const std::string& commandLine) {
    std::string s = commandLine;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if (s.find("format ") != std::string::npos || s.find("diskpart") != std::string::npos || s.find("reg delete") != std::string::npos)
        return {CommandRisk::Blocked, "command can cause destructive system changes"};
    if (s.find("del ") != std::string::npos || s.find("rmdir ") != std::string::npos || s.find("taskkill") != std::string::npos)
        return {CommandRisk::Review, "command can delete data or terminate processes"};
    if (s.find("setx ") != std::string::npos || s.find("set ") != std::string::npos)
        return {CommandRisk::Review, "command can modify environment state"};
    return {CommandRisk::Safe, "no high-risk pattern detected"};
}
}