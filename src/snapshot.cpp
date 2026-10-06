#include "handler/snapshot.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace handler {

namespace {
std::string snapshotId() {
    const auto now = std::chrono::system_clock::now();
    const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    return std::to_string(stamp);
}
}

SnapshotStore::SnapshotStore(std::filesystem::path root) : root_(std::move(root)) {}

std::optional<SnapshotInfo> SnapshotStore::create(const EnvironmentState& state) const {
    std::error_code ec;
    std::filesystem::create_directories(root_, ec);
    if (ec) return std::nullopt;

    const std::string id = snapshotId();
    const auto path = root_ / (id + ".state");
    std::ofstream out(path, std::ios::trunc);
    if (!out) return std::nullopt;

    out << "timestamp=" << state.timestampUtc << '\n'
        << "computer_name=" << state.computerName << '\n'
        << "user_name=" << state.userName << '\n'
        << "temp_path=" << state.tempPath << '\n'
        << "path=" << state.pathValue << '\n'
        << "current_directory=" << state.currentDirectory << '\n'
        << "handler_version=" << state.handlerVersion << '\n';

    if (!out.good()) return std::nullopt;
    return SnapshotInfo{id, path};
}

std::optional<EnvironmentState> SnapshotStore::load(const SnapshotInfo& snapshot) const {
    std::ifstream in(snapshot.path);
    if (!in) return std::nullopt;

    EnvironmentState state;
    std::string line;
    while (std::getline(in, line)) {
        const auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        const auto key = line.substr(0, pos);
        const auto value = line.substr(pos + 1);
        if (key == "timestamp") state.timestampUtc = value;
        else if (key == "computer_name") state.computerName = value;
        else if (key == "user_name") state.userName = value;
        else if (key == "temp_path") state.tempPath = value;
        else if (key == "path") state.pathValue = value;
        else if (key == "current_directory") state.currentDirectory = value;
        else if (key == "handler_version") state.handlerVersion = value;
    }
    return state;
}

} // namespace handler
