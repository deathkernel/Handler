#include "handler/artifact_backup.h"

#include <fstream>

namespace handler {

std::optional<ArtifactBackup> backupArtifact(
    const std::filesystem::path& source,
    const std::filesystem::path& backupRoot) {
    std::error_code ec;
    const bool exists = std::filesystem::is_regular_file(source, ec);
    if (ec) return std::nullopt;

    std::filesystem::create_directories(backupRoot, ec);
    if (ec) return std::nullopt;

    const auto name = source.filename().string();
    const auto backup = backupRoot / name;
    if (exists) {
        std::filesystem::copy_file(
            source, backup, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) return std::nullopt;
    }
    return ArtifactBackup{source, backup, exists};
}

bool restoreArtifact(const ArtifactBackup& backup) {
    std::error_code ec;
    if (backup.existed) {
        std::filesystem::copy_file(
            backup.backup, backup.original,
            std::filesystem::copy_options::overwrite_existing, ec);
        return !ec;
    }
    std::filesystem::remove(backup.original, ec);
    return !ec || !std::filesystem::exists(backup.original);
}

} // namespace handler
