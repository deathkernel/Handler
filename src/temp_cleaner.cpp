#include "handler/temp_cleaner.h"

#include <iostream>

namespace handler {

CleanupResult cleanTempDirectory(const std::filesystem::path& tempDirectory, bool dryRun) {
    CleanupResult result;
    result.dryRun = dryRun;

    if (tempDirectory.empty() || !std::filesystem::exists(tempDirectory) ||
        !std::filesystem::is_directory(tempDirectory)) return result;

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(
             tempDirectory, std::filesystem::directory_options::skip_permission_denied, ec)) {
        if (ec) {
            ++result.skipped;
            ec.clear();
            continue;
        }

        std::error_code sizeError;
        std::uintmax_t size = 0;
        if (entry.is_regular_file(sizeError) && !sizeError) {
            size = entry.file_size(sizeError);
            if (sizeError) size = 0;
        }

        ++result.candidates;
        result.candidateBytes += size;

        if (dryRun) continue;

        std::error_code removeError;
        const auto removedCount = std::filesystem::remove_all(entry.path(), removeError);
        if (removedCount > 0 && !removeError) {
            if (entry.is_directory(sizeError)) ++result.directoriesRemoved;
            else ++result.filesRemoved;
            result.bytesFreed += size;
        } else ++result.skipped;
    }
    return result;
}

void printCleanupResult(const CleanupResult& result) {
    std::cout << "TEMP Cleanup Result\n";
    std::cout << "-------------------\n";
    if (result.dryRun) {
        std::cout << "Dry-run candidates : " << result.candidates << '\n';
        std::cout << "Potential bytes    : " << result.candidateBytes << '\n';
        std::cout << "No files changed.\n";
        return;
    }
    std::cout << "Files removed      : " << result.filesRemoved << '\n';
    std::cout << "Directories removed: " << result.directoriesRemoved << '\n';
    std::cout << "Skipped/locked     : " << result.skipped << '\n';
    std::cout << "Bytes freed        : " << result.bytesFreed << '\n';
}

} // namespace handler
