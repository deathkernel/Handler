#include "handler/recovery_journal.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <string>

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

} // namespace handler

bool handler::RecoveryJournal::hasUnfinishedTransaction() const {
    std::ifstream in(file_);
    if (!in) return false;

    std::string line;
    bool active = false;
    while (std::getline(in, line)) {
        const auto delimiter = line.find(" | ");
        if (delimiter == std::string::npos) continue;
        const auto stageStart = delimiter + 3;
        const auto stageEnd = line.find(" | ", stageStart);
        if (stageEnd == std::string::npos) continue;
        const auto stage = line.substr(stageStart, stageEnd - stageStart);
        if (stage == "START" || stage == "SNAPSHOT" ||
            stage == "ACTION_BEGIN" || stage == "VERIFY_BEGIN")
            active = true;
        else if (stage == "COMMIT" || stage == "ROLLBACK" ||
                 stage == "ABORT" || stage == "RECOVERY_REQUIRED" ||
                 stage == "MANUAL_ROLLBACK" || stage == "RECOVERY_COMPLETE")
            active = false;
    }
    return active;
}
