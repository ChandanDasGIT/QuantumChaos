#include "testkit.h"

#include "appsettings.h"
#include "common/files.h"

TEST(appsettings, choices_come_back)
{
    testkit::TempFolder dir;
    const auto file = dir.path() / "settings.json";
    CHECK(appsettings::save(file, {false}));
    CHECK(!appsettings::load(file).darkTheme);
}

TEST(appsettings, missing_or_damaged_files_give_the_defaults)
{
    testkit::TempFolder dir;
    CHECK(appsettings::load(dir.path() / "missing.json") == appsettings::Settings{});
    files::write(dir.path() / "bad.json", "{not json");
    CHECK(appsettings::load(dir.path() / "bad.json") == appsettings::Settings{});
}
