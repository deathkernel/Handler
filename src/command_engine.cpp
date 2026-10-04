#include "handler/command_engine.h"

#include <cctype>
#include <sstream>

namespace handler {

std::string quoteArgument(const std::string& value) {
    if (value.empty()) return "\"\"";
    bool needsQuotes = false;
    for (const char ch : value)
        if (std::isspace(static_cast<unsigned char>(ch)) || ch == '\"') needsQuotes = true;
    if (!needsQuotes) return value;

    std::string out = "\"";
    for (const char ch : value) {
        if (ch == '\"') out += '\\\\';
        out += ch;
    }
    out += '\"';
    return out;
}

std::string buildCommandLine(const CommandSpec& command) {
    std::ostringstream out;
    out << quoteArgument(command.executable);
    for (const auto& arg : command.arguments) out << ' ' << quoteArgument(arg);
    return out.str();
}

bool isAllowedExecutable(const std::string& executable) {
    return executable == "where" || executable == "cmd" || executable == "cmake"
        || executable == "git" || executable == "python" || executable == "python.exe"
        || executable == "node" || executable == "node.exe";
}

} // namespace handler
