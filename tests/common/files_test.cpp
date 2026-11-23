#include "testkit.h"

#include "common/files.h"

#include <string>

TEST(files, writes_and_reads_back_any_bytes)
{
    testkit::TempFolder dir;
    const auto file = dir.path() / "sub" / "a.bin";
    const std::string bytes = std::string("line\none\0two\xff", 13);
    CHECK(files::write(file, bytes));
    CHECK_EQ(*files::read(file), bytes);
    CHECK(!std::filesystem::exists(dir.path() / "sub" / "a.bin.tmp"));
}

TEST(files, a_new_write_replaces_the_old_file)
{
    testkit::TempFolder dir;
    const auto file = dir.path() / "a.txt";
    CHECK(files::write(file, "first, longer text"));
    CHECK(files::write(file, "second"));
    CHECK_EQ(*files::read(file), std::string("second"));
}

TEST(files, missing_files_and_unwritable_places_fail)
{
    testkit::TempFolder dir;
    CHECK(!files::read(dir.path() / "missing").has_value());
    CHECK(files::write(dir.path() / "blocker", "x"));
    CHECK(!files::write(dir.path() / "blocker" / "inside.txt", "y"));    // a file is in the way
}
