#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

// Reading and writing whole files. A write goes to "<file>.tmp" first and then replaces the file,
// so a crash or a full disk never leaves half a file behind.
namespace files {

std::optional<std::string> read(const std::filesystem::path& file);
bool write(const std::filesystem::path& file, std::string_view bytes);

} // namespace files
