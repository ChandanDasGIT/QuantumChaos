#pragma once

#include "common/note.h"

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <set>
#include <string>
#include <vector>

namespace notes {

enum class Section { Notes, Archive, Trash, Label };

// All notes, kept in memory, and every change made to them.
class NoteList {
public:
    using Clock = std::function<std::int64_t()>;     // ms since 1970
    using IdMaker = std::function<std::string()>;

    NoteList(Clock clock, IdMaker makeId);

    const Note* find(const std::string& id) const;
    const std::vector<Note>& all() const { return m_notes; }
    // Labels made without being put on a note.
    const std::set<std::string>& ownLabels() const { return m_labels; }

    // Replaces everything with notes read back from disk; only onChanged is called.
    void load(std::vector<Note> notes, const std::vector<std::string>& ownLabels);
    // Adds the note, or replaces the one with its id; only onChanged is called.
    void put(Note note);
    // Removes the note; only onChanged is called.
    void drop(const std::string& id);

    // A new empty note (a checklist starts with one empty line). `label`, if any, is put on it.
    std::string create(bool checklist, const std::string& label = {});
    // New notes can be made in Notes and in a label, not in Archive or Trash.
    static bool canCreateIn(Section section) { return section == Section::Notes || section == Section::Label; }
    // A new note made while `section` is shown: inside a label it gets that label.
    std::string createIn(Section section, const std::string& label, bool checklist);

    void setTitle(const std::string& id, const std::string& text);
    void setBody(const std::string& id, const std::string& text);
    // Text lines become checklist lines and back.
    void setChecklist(const std::string& id, bool on);
    void setItemText(const std::string& id, int index, const std::string& text);
    void setItemDone(const std::string& id, int index, bool done);
    // Returns where the line went, or -1.
    int insertItem(const std::string& id, int index, const std::string& text);
    // Cuts line `index` at byte `at`; the part after it becomes the next line. Returns its index, or -1.
    int splitItem(const std::string& id, int index, std::size_t at);
    void removeItem(const std::string& id, int index);
    // The first line of a checklist, adding an empty one when there is none. Returns its index, or -1.
    int firstItemOrNew(const std::string& id);

    void setColor(const std::string& id, int color);
    void setPinned(const std::string& id, bool on);      // pinning takes a note out of the archive
    void setArchived(const std::string& id, bool on);    // archiving or trashing unpins it
    void setTrashed(const std::string& id, bool on);
    void toggleLabel(const std::string& id, const std::string& label);
    void createLabel(const std::string& label);
    std::vector<std::string> labels() const;

    // The labels whose name contains `typed` (any case), and whether note `id` has each.
    struct LabelChoice {
        std::string name;
        bool on = false;
    };
    std::vector<LabelChoice> labelChoices(const std::string& id, const std::string& typed) const;
    // The label name `typed` stands for: without spaces at either end.
    static std::string labelName(const std::string& typed);
    // `typed` (trimmed) is not empty and no label has that name yet.
    bool canCreateLabel(const std::string& typed) const;
    // Makes the label `typed` (trimmed) and puts it on note `id`.
    void addTypedLabel(const std::string& id, const std::string& typed);

    void erase(const std::string& id);
    void emptyTrash();
    bool discardIfEmpty(const std::string& id);

    // The ids shown in a section, newest change first. `label` is used by Section::Label; `search`
    // keeps the notes whose title, text, lines or labels contain it (any case).
    std::vector<std::string> query(Section section, const std::string& label, const std::string& search,
                                   bool pinned) const;

    // Anything changed, for the screen.
    std::function<void()> onChanged;
    // An edit made here: note `id` was made or changed, or removed for good; or the labels
    // without notes changed. load() and put() don't call these.
    std::function<void(const std::string& id)> onNoteChanged;
    std::function<void(const std::string& id)> onNoteErased;
    std::function<void()> onLabelsChanged;

private:
    enum class Part { Title, Content, Color, Pinned, Archived, Trashed, Labels };

    Note* edit(const std::string& id);
    void update(const std::string& id, std::initializer_list<Part> parts, const std::function<void(Note&)>& change);
    void addLabel(const std::string& label);
    void changed();

    std::vector<Note> m_notes;
    std::set<std::string> m_labels;
    Clock m_clock;
    IdMaker m_makeId;
};

} // namespace notes
