#include "testkit.h"

#include "common/notelist.h"

#include <string>
#include <vector>

using notes::NoteList;
using notes::Section;

namespace {

struct Fixture {
    std::int64_t now = 1000;
    int nextId = 0;
    int changes = 0;
    NoteList list{[this] { return now; }, [this] { return "id" + std::to_string(nextId++); }};
    Fixture() { list.onChanged = [this] { ++changes; }; }
};

std::vector<std::string> ids(std::initializer_list<const char*> names)
{
    std::vector<std::string> out;
    for (const char* name : names) out.emplace_back(name);
    return out;
}

} // namespace

TEST(notelist, new_notes_show_newest_first)
{
    Fixture f;
    f.list.create(false);
    f.now = 2000;
    f.list.create(false);
    CHECK(f.list.query(Section::Notes, "", "", false) == ids({"id1", "id0"}));
    f.now = 3000;
    f.list.setBody("id0", "edited");
    CHECK(f.list.query(Section::Notes, "", "", false) == ids({"id0", "id1"}));
    CHECK_EQ(f.list.find("id0")->modified, std::int64_t(3000));
}

TEST(notelist, colour_and_pin_dont_change_the_order)
{
    Fixture f;
    f.list.create(false);
    f.now = 2000;
    f.list.create(false);
    f.now = 3000;
    f.list.setColor("id0", 4);
    CHECK_EQ(f.list.find("id0")->color, 4);
    CHECK_EQ(f.list.find("id0")->modified, std::int64_t(1000));
}

TEST(notelist, pinned_notes_are_their_own_group)
{
    Fixture f;
    f.list.create(false);
    f.list.create(false);
    f.list.setPinned("id1", true);
    CHECK(f.list.query(Section::Notes, "", "", true) == ids({"id1"}));
    CHECK(f.list.query(Section::Notes, "", "", false) == ids({"id0"}));
}

TEST(notelist, archive_trash_and_pin_move_notes_between_sections)
{
    Fixture f;
    f.list.create(false);
    f.list.setPinned("id0", true);
    f.list.setArchived("id0", true);
    CHECK(!f.list.find("id0")->pinned);
    CHECK(f.list.query(Section::Notes, "", "", false).empty());
    CHECK(f.list.query(Section::Archive, "", "", false) == ids({"id0"}));
    f.list.setPinned("id0", true);                       // pinning takes it out of the archive
    CHECK(!f.list.find("id0")->archived);
    CHECK(f.list.query(Section::Notes, "", "", true) == ids({"id0"}));
    f.list.setTrashed("id0", true);
    CHECK(f.list.query(Section::Trash, "", "", false) == ids({"id0"}));
    CHECK(f.list.query(Section::Notes, "", "", true).empty());
    f.list.emptyTrash();
    CHECK(f.list.all().empty());
}

TEST(notelist, search_looks_at_title_text_lines_and_labels_in_any_case)
{
    Fixture f;
    f.list.create(false);
    f.list.setTitle("id0", "Groceries");
    f.list.create(true);
    f.list.setItemText("id1", 0, "Buy MILK");
    f.list.create(false);
    f.list.toggleLabel("id2", "Work");
    CHECK(f.list.query(Section::Notes, "", "grocer", false) == ids({"id0"}));
    CHECK(f.list.query(Section::Notes, "", "milk", false) == ids({"id1"}));
    CHECK(f.list.query(Section::Notes, "", "work", false) == ids({"id2"}));
    CHECK(f.list.query(Section::Notes, "", "nothing", false).empty());
}

TEST(notelist, labels_filter_and_list)
{
    Fixture f;
    f.list.create(false, "Ideas");
    f.list.create(false);
    f.list.createLabel("Work");
    const std::vector<std::string> expected{"Ideas", "Work"};
    CHECK(f.list.labels() == expected);
    CHECK(f.list.query(Section::Label, "Ideas", "", false) == ids({"id0"}));
    f.list.toggleLabel("id0", "Ideas");
    CHECK(f.list.query(Section::Label, "Ideas", "", false).empty());
}

TEST(notelist, checklist_converts_lines_both_ways)
{
    Fixture f;
    f.list.create(false);
    f.list.setBody("id0", "Milk\n\nBread\n");
    f.list.setChecklist("id0", true);
    const Note* n = f.list.find("id0");
    CHECK(n->checklist);
    CHECK_EQ(n->items.size(), std::size_t(2));
    CHECK_EQ(n->items[1].text, std::string("Bread"));
    CHECK(n->body.empty());
    f.list.setChecklist("id0", false);
    CHECK_EQ(f.list.find("id0")->body, std::string("Milk\nBread"));
}

TEST(notelist, checklist_lines_split_insert_and_remove)
{
    Fixture f;
    f.list.create(true);
    f.list.setItemText("id0", 0, "MilkBread");
    CHECK_EQ(f.list.splitItem("id0", 0, 4), 1);
    const Note* n = f.list.find("id0");
    CHECK_EQ(n->items[0].text, std::string("Milk"));
    CHECK_EQ(n->items[1].text, std::string("Bread"));
    CHECK_EQ(f.list.insertItem("id0", 99, "Eggs"), 2);
    f.list.setItemDone("id0", 2, true);
    CHECK(f.list.find("id0")->items[2].done);
    f.list.removeItem("id0", 0);
    CHECK_EQ(f.list.find("id0")->items.size(), std::size_t(2));
    CHECK_EQ(f.list.splitItem("id0", 9, 0), -1);
}

TEST(notelist, empty_notes_are_discarded_and_others_kept)
{
    Fixture f;
    f.list.create(true, "Ideas");
    CHECK(f.list.discardIfEmpty("id0"));
    f.list.create(false);
    f.list.setBody("id1", "kept");
    CHECK(!f.list.discardIfEmpty("id1"));
    CHECK_EQ(f.list.all().size(), std::size_t(1));
}

TEST(notelist, every_change_is_reported)
{
    Fixture f;
    f.list.create(false);
    f.list.setTitle("id0", "a");
    f.list.setColor("id0", 1);
    CHECK_EQ(f.changes, 3);
    f.list.setTitle("missing", "a");
    CHECK_EQ(f.changes, 3);
}

TEST(notelist, notes_made_inside_a_label_get_it)
{
    Fixture f;
    CHECK(NoteList::canCreateIn(Section::Notes));
    CHECK(NoteList::canCreateIn(Section::Label));
    CHECK(!NoteList::canCreateIn(Section::Archive));
    CHECK(!NoteList::canCreateIn(Section::Trash));
    f.list.createIn(Section::Label, "Work", false);
    f.list.createIn(Section::Notes, "Work", false);
    CHECK(f.list.find("id0")->labels == ids({"Work"}));
    CHECK(f.list.find("id1")->labels.empty());
}

TEST(notelist, label_choices_filter_and_show_which_are_on)
{
    Fixture f;
    f.list.create(false, "Work");
    f.list.createLabel("Ideas");
    f.list.createLabel("Homework");
    const auto choices = f.list.labelChoices("id0", "  WORK ");
    CHECK_EQ(choices.size(), std::size_t(2));
    CHECK_EQ(choices[0].name, std::string("Homework"));
    CHECK(!choices[0].on);
    CHECK_EQ(choices[1].name, std::string("Work"));
    CHECK(choices[1].on);
    CHECK_EQ(f.list.labelChoices("id0", "").size(), std::size_t(3));
}

TEST(notelist, typed_labels_are_trimmed_and_made_once)
{
    Fixture f;
    f.list.create(false);
    CHECK(!f.list.canCreateLabel("   "));
    CHECK(f.list.canCreateLabel(" Travel "));
    f.list.addTypedLabel("id0", " Travel ");
    CHECK(f.list.find("id0")->labels == ids({"Travel"}));
    CHECK(!f.list.canCreateLabel("Travel"));
    f.list.createLabel("  Ideas  ");
    const std::vector<std::string> expected{"Ideas", "Travel"};
    CHECK(f.list.labels() == expected);
}

TEST(notelist, first_checklist_line_or_a_new_one)
{
    Fixture f;
    f.list.create(true);
    CHECK_EQ(f.list.firstItemOrNew("id0"), 0);
    CHECK_EQ(f.list.find("id0")->items.size(), std::size_t(1));
    f.list.removeItem("id0", 0);
    CHECK_EQ(f.list.firstItemOrNew("id0"), 0);
    CHECK_EQ(f.list.find("id0")->items.size(), std::size_t(1));
    CHECK_EQ(f.list.firstItemOrNew("missing"), -1);
}

TEST(notelist, card_values_come_from_the_note)
{
    Note n;
    n.items = {{"a", true}, {"b", false}, {"c", false}, {"d", true}, {"e", false}};
    CHECK_EQ(n.doneCount(), 2);
    const auto open = n.openItems(2);
    CHECK_EQ(open.size(), std::size_t(2));
    CHECK_EQ(open[1].text, std::string("c"));
    CHECK(n.editable());
    n.trashed = true;
    CHECK(!n.editable());
}

TEST(notelist, label_name_drops_outer_spaces)
{
    CHECK_EQ(NoteList::labelName("  Road trip "), std::string("Road trip"));
    CHECK_EQ(NoteList::labelName("   "), std::string());
}

TEST(notelist, colours_stay_in_range)
{
    CHECK_EQ(Note::noteColorCount, 12);                       // the theme lists this many colours
    Fixture f;
    f.list.create(false);
    f.list.setColor("id0", 99);
    CHECK_EQ(f.list.find("id0")->color, Note::noteColorCount - 1);
    f.list.setColor("id0", -5);
    CHECK_EQ(f.list.find("id0")->color, 0);
    CHECK(!f.list.find("id0")->coloured());
    f.list.setColor("id0", 3);
    CHECK(f.list.find("id0")->coloured());
    Note odd;
    odd.id = "odd";
    odd.color = 40;
    f.list.put(odd);
    CHECK_EQ(f.list.find("odd")->color, Note::noteColorCount - 1);
}

TEST(notelist, each_change_stamps_its_part)
{
    Fixture f;
    f.list.create(false);
    const auto start = f.list.find("id0")->stamps;
    CHECK_EQ(start.title, std::int64_t(1000));
    CHECK_EQ(start.labels, std::int64_t(1000));
    f.now = 2000;
    f.list.setTitle("id0", "t");
    f.now = 3000;
    f.list.setItemText("id0", 0, "x");
    f.list.setBody("id0", "b");
    f.now = 4000;
    f.list.setColor("id0", 2);
    f.now = 5000;
    f.list.setArchived("id0", true);
    f.now = 6000;
    f.list.toggleLabel("id0", "Work");
    f.now = 7000;
    f.list.setTrashed("id0", true);
    const auto s = f.list.find("id0")->stamps;
    CHECK_EQ(s.title, std::int64_t(2000));
    CHECK_EQ(s.content, std::int64_t(3000));
    CHECK_EQ(s.color, std::int64_t(4000));
    CHECK_EQ(s.archived, std::int64_t(5000));
    CHECK_EQ(s.labels, std::int64_t(6000));
    CHECK_EQ(s.trashed, std::int64_t(7000));
    CHECK_EQ(s.pinned, std::int64_t(7000));                   // trashing unpins
    CHECK_EQ(f.list.find("id0")->modified, std::int64_t(3000));
}

TEST(notelist, edits_report_which_note_changed_or_went)
{
    Fixture f;
    std::vector<std::string> changed, erased;
    int labelChanges = 0;
    f.list.onNoteChanged = [&](const std::string& id) { changed.push_back(id); };
    f.list.onNoteErased = [&](const std::string& id) { erased.push_back(id); };
    f.list.onLabelsChanged = [&] { ++labelChanges; };
    f.list.create(false);
    f.list.create(false);
    f.list.setBody("id1", "x");
    f.list.setTrashed("id0", true);
    f.list.setTrashed("id1", true);
    f.list.createLabel("Work");
    f.list.createLabel("Work");
    f.list.toggleLabel("id0", "Ideas");
    f.list.emptyTrash();
    CHECK(changed == ids({"id0", "id1", "id1", "id0", "id1", "id0"}));
    CHECK(erased == ids({"id0", "id1"}));
    CHECK_EQ(labelChanges, 2);                                // Work once, then Ideas
}

TEST(notelist, load_and_put_dont_report_edits)
{
    Fixture f;
    int edits = 0;
    f.list.onNoteChanged = [&](const std::string&) { ++edits; };
    f.list.onLabelsChanged = [&] { ++edits; };
    Note a;
    a.id = "a";
    a.color = 50;
    f.list.load({a}, {"Work"});
    CHECK_EQ(f.list.all().size(), std::size_t(1));
    CHECK_EQ(f.list.find("a")->color, Note::noteColorCount - 1);
    CHECK(f.list.ownLabels().contains("Work"));
    a.title = "replaced";
    f.list.put(a);
    Note b;
    b.id = "b";
    f.list.put(b);
    CHECK_EQ(f.list.find("a")->title, std::string("replaced"));
    CHECK_EQ(f.list.all().size(), std::size_t(2));
    CHECK_EQ(edits, 0);
    CHECK_EQ(f.changes, 3);
}
