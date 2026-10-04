#include "handler/temp_cleaner.h"

#include <chrono>
#include <iostream>

namespace handler {

CleanupResult cleanTempDirectory(const std::filesystem::path& tempDirectory) {
    CleanupResult result;

    if (tempDirectory.empty() || !std::filesystem::exists(tempDirectory) ||
        !std::filesystem::is_directory(tempDirectory)) {
        return result;
    }

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
            if (sizeError) {
                size = 0;
            }
        }

        std::error_code removeError;
        const bool removed = std::filesystem::remove_all(entry.path(), removeError) > 0;

        if (removed && !removeError) {
            if (entry.is_directory(sizeError)) {
                ++result.directoriesRemoved;
            } else {
                ++result.filesRemoved;
            }
            result.bytesFreed += size;
        } else {
            ++result.skipped;
        }
    }

    return result;
}

void printCleanupResult(const CleanupResult& result) {
    std::cout << "TEMP Cleanup Result\n";
    std::cout << "-------------------\n";
    std::cout << "Files removed      : " << result.filesRemoved << '\n';
    std::cout << "Directories removed: " << result.directoriesRemoved << '\n';
    std::cout << "Skipped/locked     : " << result.skipped << '\n';
    std::cout << "Bytes freed        : " << result.bytesFreed << '\n';
}

} // namespace handler
