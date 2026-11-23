#include "testkit.h"

#include "common/files.h"
#include "common/notestore.h"

#include <string>

namespace {

const crypto::Key key = *crypto::keyFromHex(std::string(64, '1'));
const crypto::Key otherKey = *crypto::keyFromHex(std::string(64, '2'));

Note note(const std::string& id, const std::string& title)
{
    Note n;
    n.id = id;
    n.title = title;
    n.body = "multi\nline ☕";
    return n;
}

const std::string idA = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
const std::string idB = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";

} // namespace

TEST(notestore, notes_and_labels_come_back)
{
    testkit::TempFolder dir;
    {
        NoteStore store(dir.path(), key);
        CHECK(store.save(note(idA, "first")));
        CHECK(store.save(note(idB, "second")));
        CHECK(store.saveLabels({"Work", "Ideas"}));
    }
    NoteStore store(dir.path(), key);
    const auto loaded = store.load();
    CHECK_EQ(loaded.notes.size(), std::size_t(2));
    CHECK_EQ(loaded.unreadable, 0);
    CHECK(loaded.labels == std::vector<std::string>({"Work", "Ideas"}));
    for (const auto& n : loaded.notes) CHECK(n == note(n.id, n.id == idA ? "first" : "second"));
}

TEST(notestore, files_on_disk_are_encrypted)
{
    testkit::TempFolder dir;
    NoteStore store(dir.path(), key);
    store.save(note(idA, "Secret title"));
    const auto bytes = files::read(dir.path() / "notes" / (idA + ".note"));
    CHECK(bytes.has_value());
    CHECK(bytes->find("Secret") == std::string::npos);
    CHECK(bytes->find("multi") == std::string::npos);
}

TEST(notestore, removed_notes_are_gone)
{
    testkit::TempFolder dir;
    NoteStore store(dir.path(), key);
    store.save(note(idA, "a"));
    store.save(note(idB, "b"));
    CHECK(store.remove(idA));
    const auto loaded = store.load();
    CHECK_EQ(loaded.notes.size(), std::size_t(1));
    CHECK_EQ(loaded.notes[0].id, idB);
}

TEST(notestore, files_it_cant_open_are_counted_and_left_alone)
{
    testkit::TempFolder dir;
    {
        NoteStore other(dir.path(), otherKey);
        other.save(note(idA, "other key"));
    }
    files::write(dir.path() / "notes" / (idB + ".note"), "damaged");
    files::write(dir.path() / "notes" / "notanid.note", "x");
    files::write(dir.path() / "notes" / "readme.txt", "not a note file");
    NoteStore store(dir.path(), key);
    const auto loaded = store.load();
    CHECK(loaded.notes.empty());
    CHECK_EQ(loaded.unreadable, 3);
    CHECK(std::filesystem::exists(dir.path() / "notes" / (idA + ".note")));
}

TEST(notestore, a_file_renamed_to_another_note_doesnt_open)
{
    testkit::TempFolder dir;
    NoteStore store(dir.path(), key);
    store.save(note(idA, "a"));
    std::filesystem::rename(dir.path() / "notes" / (idA + ".note"), dir.path() / "notes" / (idB + ".note"));
    CHECK_EQ(store.load().unreadable, 1);
}

TEST(notestore, an_empty_folder_loads_nothing)
{
    testkit::TempFolder dir;
    NoteStore store(dir.path() / "never-made", key);
    const auto loaded = store.load();
    CHECK(loaded.notes.empty());
    CHECK(loaded.labels.empty());
    CHECK_EQ(loaded.unreadable, 0);
}

TEST(notestore, ids_that_arent_note_ids_are_refused)
{
    testkit::TempFolder dir;
    NoteStore store(dir.path(), key);
    CHECK(!store.save(note("../escape", "x")));
    CHECK(!store.remove("../escape"));
}
