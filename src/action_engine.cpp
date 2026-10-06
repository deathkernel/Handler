#include "handler/action_engine.h"

#include <array>
#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace handler {

#ifdef _WIN32

namespace {

std::wstring toWide(const std::string& value) {
    if (value.empty()) return {};
    const int size = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0) return {};

    std::wstring result(static_cast<std::size_t>(size), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), size) <= 0)
        return {};
    return result;
}

std::wstring quoteWideArgument(const std::wstring& value) {
    if (value.empty()) return L"""";

    bool needsQuotes = false;
    for (wchar_t ch : value) {
        if (ch == L' ' || ch == L'\t' || ch == L'\n' ||
            ch == L'\v' || ch == L'"')
            needsQuotes = true;
    }
    if (!needsQuotes) return value;

    std::wstring out;
    out.push_back(L'"');
    std::size_t backslashes = 0;

    for (const wchar_t ch : value) {
        if (ch == L'\') {
            ++backslashes;
            continue;
        }
        if (ch == L'"') {
            out.append(backslashes * 2 + 1, L'\');
            out.push_back(L'"');
            backslashes = 0;
            continue;
        }
        out.append(backslashes, L'\');
        backslashes = 0;
        out.push_back(ch);
    }

    out.append(backslashes * 2, L'\');
    out.push_back(L'"');
    return out;
}

std::wstring buildWideCommandLine(const CommandSpec& command) {
    std::wstring line = quoteWideArgument(toWide(command.executable));
    for (const auto& argument : command.arguments) {
        line.push_back(L' ');
        line += quoteWideArgument(toWide(argument));
    }
    return line;
}

} // namespace

ActionResult executeCommand(const CommandSpec& command) {
    if (!isAllowedExecutable(command.executable))
        return {false, -1, {}, "executable is outside Handler's allowed command set"};

    const std::wstring executable = toWide(command.executable);
    if (executable.empty())
        return {false, -1, {}, "executable is not valid UTF-8"};

    wchar_t resolved[MAX_PATH]{};
    if (SearchPathW(nullptr, executable.c_str(), L".exe",
                    MAX_PATH, resolved, nullptr) == 0)
        return {false, -1, {}, "allowlisted executable was not found on PATH"};

    SECURITY_ATTRIBUTES securityAttributes{};
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.bInheritHandle = TRUE;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &securityAttributes, 0))
        return {false, -1, {}, "failed to create output pipe"};

    if (!SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(readPipe);
        CloseHandle(writePipe);
        return {false, -1, {}, "failed to protect output pipe"};
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = writePipe;
    startup.hStdError = writePipe;

    PROCESS_INFORMATION process{};
    std::wstring commandLine = buildWideCommandLine(command);

    const BOOL created = CreateProcessW(
        resolved, commandLine.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);

    CloseHandle(writePipe);

    if (!created) {
        CloseHandle(readPipe);
        return {false, -1, {}, "failed to start process"};
    }

    std::string output;
    std::array<char, 4096> buffer{};
    DWORD bytesRead = 0;

    while (ReadFile(readPipe, buffer.data(),
                    static_cast<DWORD>(buffer.size()),
                    &bytesRead, nullptr) && bytesRead > 0)
        output.append(buffer.data(), bytesRead);

    CloseHandle(readPipe);
    WaitForSingleObject(process.hProcess, INFINITE);

    DWORD exitCode = 1;
    if (!GetExitCodeProcess(process.hProcess, &exitCode)) {
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return {true, -1, output, "failed to read process exit code"};
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    const int code = static_cast<int>(exitCode);
    return {true, code, output, code == 0 ? std::string{} : output};
}

#else

ActionResult executeCommand(const CommandSpec& command) {
    if (!isAllowedExecutable(command.executable))
        return {false, -1, {}, "executable is outside Handler's allowed command set"};

    const std::string line = buildCommandLine(command) + " 2>&1";
    FILE* pipe = popen(line.c_str(), "r");
    if (!pipe) return {false, -1, {}, "failed to start command"};

    std::string output;
    std::array<char, 512> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
        output += buffer.data();

    const int code = pclose(pipe);
    return {true, code, output, code == 0 ? std::string{} : output};
}

#endif

} // namespace handler
