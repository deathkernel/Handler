#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace handler {

struct ScheduledTask {
    std::string id;
    std::uint32_t intervalMinutes{0};
    std::string command;
    std::int64_t lastRunEpoch{0};
};

// Only known, non-destructive Handler diagnostics may run unattended.
bool isSafeScheduledCommand(const std::string& command);

class TaskScheduler {
public:
    explicit TaskScheduler(std::filesystem::path file);

    bool listTasks(std::vector<ScheduledTask>& tasks,
                   std::string& error) const;
    bool addTask(const std::string& id, std::uint32_t intervalMinutes,
                 const std::string& command, std::int64_t nowEpoch,
                 std::string& error);
    bool removeTask(const std::string& id, std::string& error);
    bool dueTasks(std::int64_t nowEpoch,
                  std::vector<ScheduledTask>& tasks,
                  std::string& error) const;
    bool markTaskRun(const std::string& id, std::int64_t nowEpoch,
                     std::string& error);

private:
    bool load(std::vector<ScheduledTask>& tasks, std::string& error) const;
    bool save(const std::vector<ScheduledTask>& tasks,
              std::string& error) const;

    std::filesystem::path file_;
};

} // namespace handler
