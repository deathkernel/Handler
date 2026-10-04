#include "handler/action_engine.h"

#include <array>
#include <cstdio>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace handler {

ActionResult executeCommand(const CommandSpec& command) {
    if (!isAllowedExecutable(command.executable))
        return {false, -1, {}, "executable is outside Handler's allowed command set"};

    const std::string line = buildCommandLine(command) + " 2>&1";
#ifdef _WIN32
    FILE* pipe = _popen(line.c_str(), "r");
#else
    FILE* pipe = popen(line.c_str(), "r");
#endif
    if (!pipe) return {false, -1, {}, "failed to start command"};

    std::string output;
    std::array<char, 512> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
        output += buffer.data();

#ifdef _WIN32
    const int code = _pclose(pipe);
#else
    const int code = pclose(pipe);
#endif
    return {true, code, output, code == 0 ? std::string{} : output};
}

} // namespace handler
