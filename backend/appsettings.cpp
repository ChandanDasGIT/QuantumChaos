#include "appsettings.h"

#include "common/files.h"
#include "common/json.h"

namespace appsettings {

Settings load(const std::filesystem::path& file)
{
    Settings settings;
    const auto bytes = files::read(file);
    if (!bytes) return settings;
    const auto parsed = json::parse(*bytes);
    if (!parsed.ok()) return settings;
    settings.darkTheme = parsed.value["darkTheme"].asBool(settings.darkTheme);
    return settings;
}

bool save(const std::filesystem::path& file, const Settings& settings)
{
    json::Value v = json::Value::object();
    v.set("darkTheme", settings.darkTheme);
    return files::write(file, json::write(v, true) + "\n");
}

} // namespace appsettings
