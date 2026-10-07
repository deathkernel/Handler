#include "handler/action_engine.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cwctype>
#ifndef _WIN32
#include <sys/types.h>
#include <cerrno>
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#endif

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

std::wstring toWide(const std::filesystem::path& value) {
    return value.empty() ? std::wstring{} : value.wstring();
}

std::wstring quoteWideArgument(const std::wstring& value) {
    if (value.empty()) return L"\"\"";

    bool needsQuotes = false;
    for (wchar_t ch : value) {
        if (ch == L' ' || ch == L'\t' || ch == L'\n' ||
            ch == L'\v' || ch == L'\"' || ch == L'\\') {
            needsQuotes = true;
            break;
        }
    }
    if (!needsQuotes) return value;

    std::wstring out;
    out.push_back(L'\"');
    std::size_t backslashes = 0;

    for (wchar_t ch : value) {
        if (ch == L'\\') {
            ++backslashes;
            continue;
        }
        if (ch == L'\"') {
            out.append(backslashes * 2 + 1, L'\\');
            out.push_back(L'\"');
            backslashes = 0;
            continue;
        }
        out.append(backslashes, L'\\');
        backslashes = 0;
        out.push_back(ch);
    }

    out.append(backslashes * 2, L'\\');
    out.push_back(L'\"');
    return out;
}

bool pathUnder(const std::filesystem::path& child, const std::filesystem::path& root) {
    std::error_code ec;
    const auto c = std::filesystem::weakly_canonical(child, ec);
    if (ec) return false;
    ec.clear();
    const auto r = std::filesystem::weakly_canonical(root, ec);
    if (ec) return false;
    auto ci = c.begin();
    auto ri = r.begin();
    for (; ri != r.end() && ci != c.end(); ++ri, ++ci) {
        if (_wcsicmp(ri->wstring().c_str(), ci->wstring().c_str()) != 0)
            return false;
    }
    return ri == r.end();
}

bool trustedWindowsExecutable(const std::filesystem::path& path) {
    std::error_code ec;
    const auto canonical = std::filesystem::weakly_canonical(path, ec);
    if (ec || canonical.empty() || !std::filesystem::is_regular_file(canonical, ec))
        return false;

    const auto current = std::filesystem::weakly_canonical(std::filesystem::current_path(), ec);
    if (!ec && pathUnder(canonical, current))
        return false;

    const auto programFiles = std::getenv("ProgramFiles");
    const auto programFiles86 = std::getenv("ProgramFiles(x86)");
    const auto localAppData = std::getenv("LOCALAPPDATA");
    const auto windowsRoot = std::getenv("WINDIR");

    std::vector<std::filesystem::path> roots;
    if (programFiles && *programFiles) roots.emplace_back(programFiles);
    if (programFiles86 && *programFiles86) roots.emplace_back(programFiles86);
    if (localAppData && *localAppData) {
        roots.emplace_back(std::filesystem::path(localAppData) / "Programs");
        roots.emplace_back(std::filesystem::path(localAppData) / "Microsoft" / "WindowsApps");
    }
    if (windowsRoot && *windowsRoot) {
        roots.emplace_back(std::filesystem::path(windowsRoot) / "System32");
        roots.emplace_back(std::filesystem::path(windowsRoot) / "SysWOW64");
    }

    for (const auto& root : roots)
        if (pathUnder(canonical, root))
            return true;
    return false;
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

    std::filesystem::path executablePath = command.executablePath;
    if (!executablePath.empty()) {
        std::error_code ec;
        const auto absolute = std::filesystem::absolute(executablePath, ec);
        if (ec || !std::filesystem::exists(absolute, ec) ||
            !std::filesystem::is_regular_file(absolute, ec)) {
            return {false, -1, {}, "configured executable path does not exist"};
        }

        const auto filename = absolute.filename().string();
        const bool validPath =
            (command.executable == "python" || command.executable == "python.exe")
                ? (filename == "python.exe")
                : (command.executable == "node" || command.executable == "node.exe")
                    ? (filename == "node.exe")
                    : (command.executable == "git")
                        ? (filename == "git.exe")
                        : (command.executable == "cmake")
                            ? (filename == "cmake.exe")
                            : (command.executable == "dotnet" || command.executable == "dotnet.exe")
                                ? (filename == "dotnet.exe")
                                : (command.executable == "npm" || command.executable == "npm.cmd")
                                    ? (filename == "npm.cmd" || filename == "npm.exe")
                                    : (command.executable == "winget" || command.executable == "winget.exe")
                                        ? (filename == "winget.exe")
                                        : (command.executable == "where")
                                            ? (filename == "where.exe")
                                            : false;
        if (!validPath)
            return {false, -1, {}, "configured executable path does not match the allowlisted tool"};

        executablePath = absolute;
    } else {
        wchar_t resolved[MAX_PATH]{};
        const std::wstring executable = toWide(command.executable);
        if (executable.empty())
            return {false, -1, {}, "executable is not valid UTF-8"};

        if (SearchPathW(nullptr, executable.c_str(), L".exe",
                        MAX_PATH, resolved, nullptr) == 0)
            return {false, -1, {}, "allowlisted executable was not found on PATH"};

        executablePath = std::filesystem::path(resolved);
        if (!trustedWindowsExecutable(executablePath))
            return {false, -1, {}, "resolved executable is outside Handler trusted installation roots"};
    }

    if (!command.workingDirectory.empty()) {
        std::error_code ec;
        if (!std::filesystem::is_directory(command.workingDirectory, ec))
            return {false, -1, {}, "working directory does not exist"};
    }

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
    const std::wstring workingDirectory = toWide(command.workingDirectory);

    const BOOL created = CreateProcessW(
        executablePath.c_str(), commandLine.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr,
        workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
        &startup, &process);

    CloseHandle(writePipe);

    if (!created) {
        CloseHandle(readPipe);
        return {false, -1, {}, "failed to start process"};
    }

    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags =
            JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        const BOOL configured = SetInformationJobObject(
            job, JobObjectExtendedLimitInformation, &limits, sizeof(limits));
        const BOOL assigned = configured && AssignProcessToJobObject(job, process.hProcess);
        if (!assigned) {
            CloseHandle(job);
            job = nullptr;
        }
    }

    std::string output;
    std::thread reader([&] {
        std::array<char, 4096> buffer{};
        DWORD bytesRead = 0;
        while (ReadFile(readPipe, buffer.data(),
                        static_cast<DWORD>(buffer.size()),
                        &bytesRead, nullptr) && bytesRead > 0)
            output.append(buffer.data(), bytesRead);
    });

    const DWORD timeout = command.timeoutMs == 0 ? 120000 : command.timeoutMs;
    const DWORD wait = WaitForSingleObject(process.hProcess, timeout);

    if (wait == WAIT_TIMEOUT) {
        if (job) {
            CloseHandle(job);
            job = nullptr;
        }
        TerminateProcess(process.hProcess, 124);
        WaitForSingleObject(process.hProcess, 5000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        if (reader.joinable()) reader.join();
        CloseHandle(readPipe);
        return {true, 124, output, "command timed out and was terminated"};
    }

    DWORD exitCode = 1;
    const BOOL exitRead = GetExitCodeProcess(process.hProcess, &exitCode);

    if (job) CloseHandle(job);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    if (reader.joinable()) reader.join();
    CloseHandle(readPipe);

    if (!exitRead)
        return {true, -1, output, "failed to read process exit code"};

    const int code = static_cast<int>(exitCode);
    return {true, code, output, code == 0 ? std::string{} : output};
}

#else

namespace {
std::string shellQuote(const std::string& value) {
    std::string out = "'";
    for (char ch : value) {
        if (ch == '\'') out += "'\\''";
        else out += ch;
    }
    out += "'";
    return out;
}

std::string buildShellCommand(const CommandSpec& command) {
    std::string line = shellQuote(command.executable);
    for (const auto& arg : command.arguments)
        line += " " + shellQuote(arg);
    return line;
}
}

ActionResult executeCommand(const CommandSpec& command) {
    if (!isAllowedExecutable(command.executable))
        return {false, -1, {}, "executable is outside Handler's allowed command set"};

    if (!command.workingDirectory.empty()) {
        std::error_code ec;
        if (!std::filesystem::is_directory(command.workingDirectory, ec))
            return {false, -1, {}, "working directory does not exist"};
    }

    int outputPipe[2]{};
    if (pipe(outputPipe) != 0)
        return {false, -1, {}, "failed to create output pipe"};

    const pid_t child = fork();
    if (child < 0) {
        close(outputPipe[0]);
        close(outputPipe[1]);
        return {false, -1, {}, "failed to fork command process"};
    }

    if (child == 0) {
        setpgid(0, 0);
        close(outputPipe[0]);
        if (!command.workingDirectory.empty())
            (void)chdir(command.workingDirectory.c_str());
        dup2(outputPipe[1], STDOUT_FILENO);
        dup2(outputPipe[1], STDERR_FILENO);
        close(outputPipe[1]);

        const std::string line = buildShellCommand(command);
        execl("/bin/sh", "sh", "-c", line.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    close(outputPipe[1]);
    std::string output;
    std::thread reader([&] {
        std::array<char, 4096> buffer{};
        ssize_t count = 0;
        while ((count = read(outputPipe[0], buffer.data(), buffer.size())) > 0)
            output.append(buffer.data(), static_cast<std::size_t>(count));
    });

    const auto timeout = std::chrono::milliseconds(
        command.timeoutMs == 0 ? 120000 : command.timeoutMs);
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true) {
        int status = 0;
        const pid_t result = waitpid(child, &status, WNOHANG);
        if (result == child) {
            if (reader.joinable()) reader.join();
            close(outputPipe[0]);
            const int code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
            return {true, code, output, code == 0 ? std::string{} : output};
        }
        if (result < 0) {
            kill(child, SIGKILL);
            waitpid(child, &status, 0);
            if (reader.joinable()) reader.join();
            close(outputPipe[0]);
            return {true, -1, output, "failed to wait for command"};
        }

        if (std::chrono::steady_clock::now() >= deadline) {
            kill(-child, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            int finalStatus = 0;
            if (waitpid(child, &finalStatus, WNOHANG) == 0) {
                kill(-child, SIGKILL);
                waitpid(child, &finalStatus, 0);
            }
            if (reader.joinable()) reader.join();
            close(outputPipe[0]);
            return {true, 124, output, "command timed out and was terminated"};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
#endif

} // namespace handler
