#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace handler {

struct JournalEntry {
    std::string timestampUtc;
    std::string stage;
    std::string details;
};

class RecoveryJournal {
public:
    explicit RecoveryJournal(std::filesystem::path file);
    bool record(const std::string& stage, const std::string& details) const;
    std::vector<JournalEntry> read() const;
    const std::filesystem::path& file() const noexcept { return file_; }

private:
    std::filesystem::path file_;
};

} // namespace handler
