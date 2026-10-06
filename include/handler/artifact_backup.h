#pragma once

#include <filesystem>
#include <optional>

namespace handler {

struct ArtifactBackup {
    std::filesystem::path original;
    std::filesystem::path backup;
    bool existed{false};
};

std::optional<ArtifactBackup> backupArtifact(
    const std::filesystem::path& source,
    const std::filesystem::path& backupRoot);

bool restoreArtifact(const ArtifactBackup& backup);

} // namespace handler
