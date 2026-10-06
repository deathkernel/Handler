#include "handler/recovery_journal.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace handler {

RecoveryJournal::RecoveryJournal(std::filesystem::path file) : file_(std::move(file)) {}

bool RecoveryJournal::record(const std::string& stage, const std::string& details) const {
    std::error_code ec;
    if (!file_.parent_path().empty())
        std::filesystem::create_directories(file_.parent_path(), ec);
    if (ec) return false;

    std::ofstream out(file_, std::ios::app);
    if (!out) return false;

    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ")
        << " | " << stage << " | " << details << '\n';
    return out.good();
}

std::vector<JournalEntry> RecoveryJournal::read() const {
    std::vector<JournalEntry> entries;
    std::ifstream in(file_);
    if (!in) return entries;

    std::string line;
    while (std::getline(in, line)) {
        const auto first = line.find(" | ");
        if (first == std::string::npos) continue;
        const auto second = line.find(" | ", first + 3);
        if (second == std::string::npos) continue;

        entries.push_back({
            line.substr(0, first),
            line.substr(first + 3, second - first - 3),
            line.substr(second + 3)
        });
    }
    return entries;
}

} // namespace handler
