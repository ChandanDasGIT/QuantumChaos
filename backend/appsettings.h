#pragma once

#include <filesystem>

// The choices that outlive a run, kept as plain JSON (nothing in it is secret).
namespace appsettings {

struct Settings {
    bool darkTheme = true;
    bool operator==(const Settings&) const = default;
};

// The defaults when the file is missing or can't be read.
Settings load(const std::filesystem::path& file);
bool save(const std::filesystem::path& file, const Settings& settings);

} // namespace appsettings
