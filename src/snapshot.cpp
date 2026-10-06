#include "handler/snapshot.h"

#include <chrono>
#include <fstream>
#include <sstream>
#include <utility>
#include <algorithm>

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

    const std::string baseId = snapshotId();
    std::string id = baseId;
    std::size_t suffix = 0;
    std::error_code existsEc;
    while (std::filesystem::exists(root_ / (id + ".state"), existsEc)) {
        if (existsEc) return std::nullopt;
        id = baseId + "-" + std::to_string(++suffix);
    }
    const auto path = root_ / (id + ".state");
    std::ofstream out(path, std::ios::trunc);
    if (!out) return std::nullopt;

    out << "timestamp_utc=" << state.timestampUtc << '\n'
        << "computer_name=" << state.computerName << '\n'
        << "user_name=" << state.userName << '\n'
        << "temp_path=" << state.tempPath << '\n'
        << "path=" << state.pathValue << '\n'
        << "current_directory=" << state.currentDirectory.string() << '\n'
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

        if (key == "timestamp_utc") state.timestampUtc = value;
        else if (key == "computer_name") state.computerName = value;
        else if (key == "user_name") state.userName = value;
        else if (key == "temp_path") state.tempPath = value;
        else if (key == "path") state.pathValue = value;
        else if (key == "current_directory") state.currentDirectory = value;
        else if (key == "handler_version") state.handlerVersion = value;
    }
    return state;
}

std::vector<SnapshotInfo> SnapshotStore::list() const {
    std::vector<SnapshotInfo> snapshots;
    std::error_code ec;
    if (!std::filesystem::exists(root_, ec)) return snapshots;
    for (const auto& entry : std::filesystem::directory_iterator(root_, ec)) {
        if (ec) break;
        if (!entry.is_regular_file() || entry.path().extension() != ".state") continue;
        snapshots.push_back({entry.path().stem().string(), entry.path()});
    }
    std::sort(snapshots.begin(), snapshots.end(),
              [](const auto& a, const auto& b) { return a.id > b.id; });
    return snapshots;
}

std::optional<SnapshotInfo> SnapshotStore::find(const std::string& id) const {
    if (id.empty()) return std::nullopt;
    const auto path = root_ / (id + ".state");
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) return std::nullopt;
    return SnapshotInfo{id, path};
}

} // namespace handler
