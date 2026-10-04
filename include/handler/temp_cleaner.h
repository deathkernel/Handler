#pragma once

#include <cstddef>
#include <filesystem>\n#include <cstdint>

namespace handler {

struct CleanupResult {
    std::size_t filesRemoved{0};
    std::size_t directoriesRemoved{0};
    std::size_t skipped{0};
    std::uintmax_t bytesFreed{0};
};

CleanupResult cleanTempDirectory(const std::filesystem::path& tempDirectory);

void printCleanupResult(const CleanupResult& result);

} // namespace handler
