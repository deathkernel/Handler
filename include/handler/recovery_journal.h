#pragma once

#include <filesystem>
#include <string>

namespace handler {

class RecoveryJournal {
public:
    explicit RecoveryJournal(std::filesystem::path file);
    bool record(const std::string& stage, const std::string& details) const;
    bool hasUnfinishedTransaction() const;

private:
    std::filesystem::path file_;
};

} // namespace handler
