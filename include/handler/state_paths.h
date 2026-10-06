#pragma once
#include <filesystem>

namespace handler {

// Single source of truth for Handler-owned persistent state.
std::filesystem::path handlerStateRoot();
std::filesystem::path handlerTransactionRoot();

} // namespace handler
