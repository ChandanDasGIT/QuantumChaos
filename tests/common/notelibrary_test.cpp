#include "testkit.h"

#include "common/notelibrary.h"

#include <fstream>
#include <string>

namespace {

const crypto::Key key = *crypto::keyFromHex(std::string(64, '3'));

std::int64_t now = 1000;
std::int64_t testClock() { return now; }

} // namespace

TEST(notelibrary, every_edit_is_on_disk_at_once)
{
    testkit::TempFolder dir;
    std::string id;
    {
        NoteLibrary library(dir.path(), key, testClock);
        id = library.list().create(true, "Work");
        library.list().setTitle(id, "Groceries");
        library.list().setItemText(id, 0, "Milk");
        library.list().insertItem(id, 1, "Bread");
        library.list().setColor(id, 4);
        library.list().createLabel("Ideas");
    }
    NoteLibrary library(dir.path(), key, testClock);
    const Note* n = library.list().find(id);
    CHECK(n != nullptr);
    CHECK_EQ(n->title, std::string("Groceries"));
    CHECK_EQ(n->items.size(), std::size_t(2));
    CHECK_EQ(n->items[1].text, std::string("Bread"));
    CHECK_EQ(n->color, 4);
    CHECK(n->labels == std::vector<std::string>({"Work"}));
    CHECK(library.list().ownLabels().contains("Ideas"));
    CHECK(!library.saveFailed());
    CHECK_EQ(library.unreadable(), 0);
}

TEST(notelibrary, deleted_and_discarded_notes_stay_gone)
{
    testkit::TempFolder dir;
    std::string kept;
    {
        NoteLibrary library(dir.path(), key, testClock);
        const std::string empty = library.list().create(false);
        library.list().discardIfEmpty(empty);
        const std::string trashed = library.list().create(false);
        library.list().setBody(trashed, "bin me");
        library.list().setTrashed(trashed, true);
        library.list().emptyTrash();
        kept = library.list().create(false);
        library.list().setBody(kept, "keep me");
    }
    NoteLibrary library(dir.path(), key, testClock);
    CHECK_EQ(library.list().all().size(), std::size_t(1));
    CHECK(library.list().find(kept) != nullptr);
}

TEST(notelibrary, a_failed_write_is_reported)
{
    testkit::TempFolder dir;
    std::filesystem::create_directories(dir.path());
    {
        std::ofstream blocker(dir.path() / "notes");                    // a file where the folder should be
    }
    NoteLibrary library(dir.path(), key, testClock);
    library.list().create(false);
    CHECK(library.saveFailed());
}
