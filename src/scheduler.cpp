#include "handler/scheduler.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace handler {
namespace {

constexpr std::uint32_t kMaximumIntervalMinutes = 7U * 24U * 60U;

bool validId(const std::string& id) {
    if (id.empty() || id.size() > 48) return false;
    return std::all_of(id.begin(), id.end(), [](unsigned char ch) {
        return std::isalnum(ch) || ch == '-' || ch == '_';
    });
}

bool parseUnsigned(const std::string& text, std::uint32_t& value) {
    if (text.empty()) return false;
    std::uint64_t parsed = 0;
    for (const unsigned char ch : text) {
        if (!std::isdigit(ch)) return false;
        parsed = parsed * 10U + static_cast<std::uint64_t>(ch - '0');
        if (parsed > std::numeric_limits<std::uint32_t>::max()) return false;
    }
    value = static_cast<std::uint32_t>(parsed);
    return true;
}

bool parseSigned(const std::string& text, std::int64_t& value) {
    if (text.empty()) return false;
    std::istringstream input(text);
    input >> value;
    return input && input.peek() == std::char_traits<char>::eof();
}

std::vector<std::string> splitRecord(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (true) {
        const auto separator = line.find('|', start);
        if (separator == std::string::npos) {
            fields.push_back(line.substr(start));
            break;
        }
        fields.push_back(line.substr(start, separator - start));
        start = separator + 1;
    }
    return fields;
}

bool validTask(const ScheduledTask& task) {
    return validId(task.id) &&
           task.intervalMinutes >= 1 &&
           task.intervalMinutes <= kMaximumIntervalMinutes &&
           isSafeScheduledCommand(task.command) &&
           task.lastRunEpoch >= 0;
}

} // namespace

bool isSafeScheduledCommand(const std::string& command) {
    // Keep unattended execution deliberately narrow. Anything that installs,
    // removes, repairs, cleans, rolls back, or launches an arbitrary process
    // must stay on the interactive path and use its existing approval gates.
    static const std::set<std::string> allowed{
        "health", "self-check", "state", "status", "history", "version",
        "modules", "doctor", "updates", "protect", "discover", "project",
        "graph"
    };
    return allowed.find(command) != allowed.end();
}

TaskScheduler::TaskScheduler(std::filesystem::path file)
    : file_(std::move(file)) {}

bool TaskScheduler::load(std::vector<ScheduledTask>& tasks,
                         std::string& error) const {
    tasks.clear();
    error.clear();

    std::error_code ec;
    const bool exists = std::filesystem::exists(file_, ec);
    if (ec) {
        error = "Cannot inspect schedule file: " + ec.message();
        return false;
    }
    if (!exists) return true;

    std::ifstream input(file_);
    if (!input) {
        error = "Cannot open schedule file for reading.";
        return false;
    }

    std::set<std::string> seen;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (line.empty()) continue;

        const auto fields = splitRecord(line);
        ScheduledTask task;
        std::uint32_t interval = 0;
        if (fields.size() != 4 ||
            !parseUnsigned(fields[1], interval) ||
            !parseSigned(fields[3], task.lastRunEpoch)) {
            error = "Invalid schedule record at line " +
                    std::to_string(lineNumber) + ".";
            tasks.clear();
            return false;
        }

        task.id = fields[0];
        task.intervalMinutes = interval;
        task.command = fields[2];
        if (!validTask(task) || !seen.insert(task.id).second) {
            error = "Invalid or duplicate schedule record at line " +
                    std::to_string(lineNumber) + ".";
            tasks.clear();
            return false;
        }
        tasks.push_back(std::move(task));
    }

    if (input.bad()) {
        error = "Error while reading schedule file.";
        tasks.clear();
        return false;
    }
    return true;
}

bool TaskScheduler::save(const std::vector<ScheduledTask>& tasks,
                         std::string& error) const {
    error.clear();
    std::error_code ec;
    if (!file_.parent_path().empty())
        std::filesystem::create_directories(file_.parent_path(), ec);
    if (ec) {
        error = "Cannot create schedule directory: " + ec.message();
        return false;
    }

    const auto temporary = std::filesystem::path(file_.wstring() + L".tmp");
    {
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) {
            error = "Cannot create temporary schedule file.";
            return false;
        }
        for (const auto& task : tasks) {
            if (!validTask(task)) {
                error = "Refusing to save an invalid scheduled task.";
                output.close();
                std::filesystem::remove(temporary, ec);
                return false;
            }
            output << task.id << '|' << task.intervalMinutes << '|'
                   << task.command << '|' << task.lastRunEpoch << '\n';
        }
        output.flush();
        if (!output) {
            error = "Failed while writing schedule file.";
            output.close();
            std::filesystem::remove(temporary, ec);
            return false;
        }
    }

#ifdef _WIN32
    if (!MoveFileExW(temporary.c_str(), file_.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = "Cannot replace schedule file (Windows error " +
                std::to_string(GetLastError()) + ").";
        std::filesystem::remove(temporary, ec);
        return false;
    }
#else
    std::filesystem::rename(temporary, file_, ec);
    if (ec) {
        error = "Cannot replace schedule file: " + ec.message();
        std::filesystem::remove(temporary, ec);
        return false;
    }
#endif
    return true;
}

bool TaskScheduler::listTasks(std::vector<ScheduledTask>& tasks,
                              std::string& error) const {
    return load(tasks, error);
}

bool TaskScheduler::addTask(const std::string& id,
                            std::uint32_t intervalMinutes,
                            const std::string& command,
                            std::int64_t nowEpoch,
                            std::string& error) {
    error.clear();
    if (!validId(id)) {
        error = "Invalid task ID. Use 1-48 letters, digits, '-' or '_'.";
        return false;
    }
    if (intervalMinutes < 1 || intervalMinutes > kMaximumIntervalMinutes) {
        error = "Interval must be between 1 and 10080 minutes (7 days).";
        return false;
    }
    if (!isSafeScheduledCommand(command)) {
        error = "Command is not approved for unattended execution. "
                "Use an interactive Handler command for repairs, cleanup, "
                "installs, removals, rollback, or arbitrary commands.";
        return false;
    }
    if (nowEpoch < 0) {
        error = "Invalid current time.";
        return false;
    }

    std::vector<ScheduledTask> tasks;
    if (!load(tasks, error)) return false;
    if (std::any_of(tasks.begin(), tasks.end(), [&](const ScheduledTask& task) {
            return task.id == id;
        })) {
        error = "A scheduled task with that ID already exists.";
        return false;
    }

    tasks.push_back({id, intervalMinutes, command, nowEpoch});
    return save(tasks, error);
}

bool TaskScheduler::removeTask(const std::string& id, std::string& error) {
    error.clear();
    if (!validId(id)) {
        error = "Invalid task ID.";
        return false;
    }

    std::vector<ScheduledTask> tasks;
    if (!load(tasks, error)) return false;
    const auto originalSize = tasks.size();
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                               [&](const ScheduledTask& task) {
                                   return task.id == id;
                               }),
                tasks.end());
    if (tasks.size() == originalSize) {
        error = "Scheduled task not found.";
        return false;
    }
    return save(tasks, error);
}

bool TaskScheduler::dueTasks(std::int64_t nowEpoch,
                             std::vector<ScheduledTask>& tasks,
                             std::string& error) const {
    std::vector<ScheduledTask> all;
    if (!load(all, error)) {
        tasks.clear();
        return false;
    }

    tasks.clear();
    for (const auto& task : all) {
        if (nowEpoch >= task.lastRunEpoch &&
            nowEpoch - task.lastRunEpoch >=
                static_cast<std::int64_t>(task.intervalMinutes) * 60) {
            tasks.push_back(task);
        }
    }
    return true;
}

bool TaskScheduler::markTaskRun(const std::string& id,
                                std::int64_t nowEpoch,
                                std::string& error) {
    error.clear();
    if (nowEpoch < 0) {
        error = "Invalid current time.";
        return false;
    }

    std::vector<ScheduledTask> tasks;
    if (!load(tasks, error)) return false;
    const auto found = std::find_if(tasks.begin(), tasks.end(),
                                    [&](const ScheduledTask& task) {
                                        return task.id == id;
                                    });
    if (found == tasks.end()) {
        error = "Scheduled task not found.";
        return false;
    }
    found->lastRunEpoch = nowEpoch;
    return save(tasks, error);
}

} // namespace handler
