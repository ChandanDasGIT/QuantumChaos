#include "common/notelist.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace notes {

namespace {

std::string lower(std::string s)
{
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool blank(const std::string& s)
{
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c) != 0; });
}

std::string trimmed(const std::string& s)
{
    const auto first = std::find_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c) == 0; });
    const auto last = std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return std::isspace(c) == 0; }).base();
    return first < last ? std::string(first, last) : std::string();
}

bool contains(const std::string& text, const std::string& needle)
{
    return lower(text).find(needle) != std::string::npos;
}

bool matches(const Note& n, const std::string& needle)
{
    if (needle.empty()) return true;
    if (contains(n.title, needle) || contains(n.body, needle)) return true;
    for (const auto& item : n.items)
        if (contains(item.text, needle)) return true;
    for (const auto& label : n.labels)
        if (contains(label, needle)) return true;
    return false;
}

bool inSection(const Note& n, Section section, const std::string& label)
{
    switch (section) {
    case Section::Notes: return !n.archived && !n.trashed;
    case Section::Archive: return n.archived && !n.trashed;
    case Section::Trash: return n.trashed;
    case Section::Label:
        return !n.archived && !n.trashed && std::find(n.labels.begin(), n.labels.end(), label) != n.labels.end();
    }
    return false;
}

} // namespace

NoteList::NoteList(Clock clock, IdMaker makeId) : m_clock(std::move(clock)), m_makeId(std::move(makeId)) {}

const Note* NoteList::find(const std::string& id) const
{
    const auto it = std::find_if(m_notes.begin(), m_notes.end(), [&](const Note& n) { return n.id == id; });
    return it == m_notes.end() ? nullptr : &*it;
}

Note* NoteList::edit(const std::string& id)
{
    return const_cast<Note*>(std::as_const(*this).find(id));
}

void NoteList::changed()
{
    if (onChanged) onChanged();
}

void NoteList::update(const std::string& id, std::initializer_list<Part> parts, const std::function<void(Note&)>& change)
{
    Note* n = edit(id);
    if (!n) return;
    change(*n);
    const std::int64_t now = m_clock();
    for (const Part part : parts) {
        switch (part) {
        case Part::Title: n->stamps.title = n->modified = now; break;
        case Part::Content: n->stamps.content = n->modified = now; break;
        case Part::Color: n->stamps.color = now; break;
        case Part::Pinned: n->stamps.pinned = now; break;
        case Part::Archived: n->stamps.archived = now; break;
        case Part::Trashed: n->stamps.trashed = now; break;
        case Part::Labels: n->stamps.labels = now; break;
        }
    }
    if (onNoteChanged) onNoteChanged(id);
    changed();
}

void NoteList::addLabel(const std::string& label)
{
    if (!m_labels.insert(label).second) return;
    if (onLabelsChanged) onLabelsChanged();
}

void NoteList::load(std::vector<Note> notes, const std::vector<std::string>& ownLabels)
{
    m_notes = std::move(notes);
    for (auto& n : m_notes) n.color = std::clamp(n.color, 0, Note::noteColorCount - 1);
    m_labels = {ownLabels.begin(), ownLabels.end()};
    changed();
}

void NoteList::put(Note note)
{
    note.color = std::clamp(note.color, 0, Note::noteColorCount - 1);
    if (Note* n = edit(note.id))
        *n = std::move(note);
    else
        m_notes.push_back(std::move(note));
    changed();
}

std::string NoteList::create(bool checklist, const std::string& label)
{
    Note n;
    n.id = m_makeId();
    n.checklist = checklist;
    if (checklist) n.items.push_back({});
    if (!blank(label)) n.labels.push_back(label);
    const std::int64_t now = m_clock();
    n.created = n.modified = now;
    n.stamps = {now, now, now, now, now, now, now};
    m_notes.push_back(n);
    if (onNoteChanged) onNoteChanged(n.id);
    changed();
    return n.id;
}

std::string NoteList::createIn(Section section, const std::string& label, bool checklist)
{
    return create(checklist, section == Section::Label ? label : std::string());
}

void NoteList::setTitle(const std::string& id, const std::string& text)
{
    update(id, {Part::Title}, [&](Note& n) { n.title = text; });
}

void NoteList::setBody(const std::string& id, const std::string& text)
{
    update(id, {Part::Content}, [&](Note& n) { n.body = text; });
}

void NoteList::setChecklist(const std::string& id, bool on)
{
    update(id, {Part::Content}, [&](Note& n) {
        if (n.checklist == on) return;
        if (on) {
            n.items.clear();
            std::size_t start = 0;
            while (start <= n.body.size()) {
                std::size_t end = n.body.find('\n', start);
                if (end == std::string::npos) end = n.body.size();
                const std::string line = n.body.substr(start, end - start);
                if (!blank(line)) n.items.push_back({line, false});
                start = end + 1;
            }
            if (n.items.empty()) n.items.push_back({});
            n.body.clear();
        } else {
            std::string text;
            for (const auto& item : n.items) {
                if (!text.empty()) text += '\n';
                text += item.text;
            }
            n.body = text;
            n.items.clear();
        }
        n.checklist = on;
    });
}

void NoteList::setItemText(const std::string& id, int index, const std::string& text)
{
    update(id, {Part::Content}, [&](Note& n) {
        if (index >= 0 && index < static_cast<int>(n.items.size())) n.items[index].text = text;
    });
}

void NoteList::setItemDone(const std::string& id, int index, bool done)
{
    update(id, {Part::Content}, [&](Note& n) {
        if (index >= 0 && index < static_cast<int>(n.items.size())) n.items[index].done = done;
    });
}

int NoteList::insertItem(const std::string& id, int index, const std::string& text)
{
    int at = -1;
    update(id, {Part::Content}, [&](Note& n) {
        at = std::clamp(index, 0, static_cast<int>(n.items.size()));
        n.items.insert(n.items.begin() + at, ChecklistItem{text, false});
    });
    return at;
}

int NoteList::splitItem(const std::string& id, int index, std::size_t at)
{
    int added = -1;
    update(id, {Part::Content}, [&](Note& n) {
        if (index < 0 || index >= static_cast<int>(n.items.size())) return;
        std::string& text = n.items[index].text;
        const std::size_t cut = std::min(at, text.size());
        const std::string rest = text.substr(cut);
        text.erase(cut);
        n.items.insert(n.items.begin() + index + 1, ChecklistItem{rest, false});
        added = index + 1;
    });
    return added;
}

void NoteList::removeItem(const std::string& id, int index)
{
    update(id, {Part::Content}, [&](Note& n) {
        if (index >= 0 && index < static_cast<int>(n.items.size())) n.items.erase(n.items.begin() + index);
    });
}

int NoteList::firstItemOrNew(const std::string& id)
{
    const Note* n = find(id);
    if (!n) return -1;
    if (!n->items.empty()) return 0;
    return insertItem(id, 0, {});
}

void NoteList::setColor(const std::string& id, int color)
{
    update(id, {Part::Color}, [&](Note& n) { n.color = std::clamp(color, 0, Note::noteColorCount - 1); });
}

void NoteList::setPinned(const std::string& id, bool on)
{
    update(id, {Part::Pinned, Part::Archived}, [&](Note& n) {
        n.pinned = on;
        if (on) n.archived = false;
    });
}

void NoteList::setArchived(const std::string& id, bool on)
{
    update(id, {Part::Archived, Part::Pinned}, [&](Note& n) {
        n.archived = on;
        if (on) n.pinned = false;
    });
}

void NoteList::setTrashed(const std::string& id, bool on)
{
    update(id, {Part::Trashed, Part::Pinned}, [&](Note& n) {
        n.trashed = on;
        if (on) n.pinned = false;
    });
}

void NoteList::toggleLabel(const std::string& id, const std::string& typed)
{
    const std::string label = trimmed(typed);
    if (label.empty()) return;
    addLabel(label);
    update(id, {Part::Labels}, [&](Note& n) {
        const auto it = std::find(n.labels.begin(), n.labels.end(), label);
        if (it == n.labels.end())
            n.labels.push_back(label);
        else
            n.labels.erase(it);
    });
}

void NoteList::createLabel(const std::string& typed)
{
    const std::string label = trimmed(typed);
    if (label.empty() || m_labels.contains(label)) return;
    addLabel(label);
    changed();
}

std::vector<std::string> NoteList::labels() const
{
    std::set<std::string> all = m_labels;
    for (const auto& n : m_notes) all.insert(n.labels.begin(), n.labels.end());
    return {all.begin(), all.end()};
}

std::vector<NoteList::LabelChoice> NoteList::labelChoices(const std::string& id, const std::string& typed) const
{
    const Note* n = find(id);
    const std::string needle = lower(trimmed(typed));
    std::vector<LabelChoice> choices;
    for (const auto& label : labels()) {
        if (!contains(label, needle)) continue;
        const bool on = n && std::find(n->labels.begin(), n->labels.end(), label) != n->labels.end();
        choices.push_back({label, on});
    }
    return choices;
}

std::string NoteList::labelName(const std::string& typed)
{
    return trimmed(typed);
}

bool NoteList::canCreateLabel(const std::string& typed) const
{
    const std::string name = trimmed(typed);
    if (name.empty()) return false;
    const auto all = labels();
    return std::find(all.begin(), all.end(), name) == all.end();
}

void NoteList::addTypedLabel(const std::string& id, const std::string& typed)
{
    if (!canCreateLabel(typed)) return;
    toggleLabel(id, trimmed(typed));
}

void NoteList::drop(const std::string& id)
{
    if (std::erase_if(m_notes, [&](const Note& n) { return n.id == id; }) > 0) changed();
}

void NoteList::erase(const std::string& id)
{
    if (std::erase_if(m_notes, [&](const Note& n) { return n.id == id; }) == 0) return;
    if (onNoteErased) onNoteErased(id);
    changed();
}

void NoteList::emptyTrash()
{
    std::vector<std::string> trashed;
    for (const auto& n : m_notes)
        if (n.trashed) trashed.push_back(n.id);
    for (const auto& id : trashed) erase(id);
}

bool NoteList::discardIfEmpty(const std::string& id)
{
    const Note* n = find(id);
    if (!n || !n->isEmpty()) return false;
    erase(id);
    return true;
}

std::vector<std::string> NoteList::query(Section section, const std::string& label, const std::string& search,
                                         bool pinned) const
{
    const std::string needle = lower(search);
    std::vector<const Note*> hits;
    for (const auto& n : m_notes)
        if (inSection(n, section, label) && n.pinned == pinned && matches(n, needle)) hits.push_back(&n);
    std::stable_sort(hits.begin(), hits.end(), [](const Note* a, const Note* b) {
        return a->modified != b->modified ? a->modified > b->modified : a->created > b->created;
    });
    std::vector<std::string> ids;
    for (const Note* n : hits) ids.push_back(n->id);
    return ids;
}

} // namespace notes
