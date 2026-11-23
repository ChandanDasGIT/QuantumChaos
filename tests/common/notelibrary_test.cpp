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

TEST(notelibrary, changes_from_another_device_say_what_to_read_again)
{
    testkit::TempFolder dir;
    NoteLibrary library(dir.path(), key, testClock);
    const std::string id = library.list().create(false);
    library.list().setTitle(id, "t");
    library.list().setBody(id, "b");
    std::vector<std::pair<bool, bool>> changes;
    std::vector<std::string> removed;
    library.onRemoteChanged = [&](const std::string&, NoteLibrary::RemoteChange c) { changes.push_back({c.title, c.content}); };
    library.onRemoteRemoved = [&](const std::string& gone) { removed.push_back(gone); };

    Note remote = *library.list().find(id);
    remote.title = "new title";
    library.applyRemote(remote);
    remote.body = "new body";
    library.applyRemote(remote);
    remote.color = 3;                                                   // nothing to read again
    library.applyRemote(remote);
    remote.checklist = true;
    remote.items = {{"line", false}};
    library.applyRemote(remote);
    library.removeRemote(id);
    library.removeRemote(id);                                           // already gone: no second report

    CHECK_EQ(changes.size(), std::size_t(3));
    CHECK(changes[0] == std::make_pair(true, false));
    CHECK(changes[1] == std::make_pair(false, true));
    CHECK(changes[2] == std::make_pair(false, true));
    CHECK(removed == std::vector<std::string>({id}));
    CHECK(library.list().find(id) == nullptr);
}
