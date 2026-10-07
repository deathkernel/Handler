#include "handler/artifact_backup.h"

#include <fstream>
#include <functional>
#include <iomanip>
#include <sstream>

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
    const auto canonicalSource = std::filesystem::weakly_canonical(source, ec);
    if (ec) return std::nullopt;
    const auto digest = std::hash<std::string>{}(canonicalSource.string());
    std::ostringstream suffix;
    suffix << std::hex << digest;
    const auto backup = backupRoot / (name + "." + suffix.str() + ".bak");
    if (exists) {
        std::filesystem::copy_file(
            source, backup, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) return std::nullopt;
    }
    std::uintmax_t size = 0;
    if (exists) {
        size = std::filesystem::file_size(source, ec);
        if (ec) return std::nullopt;
    }
    return ArtifactBackup{source, backup, exists, size, exists ? size : 0};
}

bool restoreArtifact(const ArtifactBackup& backup) {
    std::error_code ec;
    if (backup.existed) {
        if (!std::filesystem::is_regular_file(backup.backup, ec) || ec) return false;
        const auto backupSize = std::filesystem::file_size(backup.backup, ec);
        if (ec || backupSize != backup.backupSize) return false;
        std::filesystem::copy_file(
            backup.backup, backup.original,
            std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) return false;
        const auto restoredSize = std::filesystem::file_size(backup.original, ec);
        return !ec && restoredSize == backup.originalSize;
    }
    std::filesystem::remove(backup.original, ec);
    return !ec || !std::filesystem::exists(backup.original);
}

} // namespace handler
