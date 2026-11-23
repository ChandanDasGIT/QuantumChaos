#include "common/notemerge.h"

#include <algorithm>
#include <string>

namespace notemerge {

namespace {

// The content as one comparable value: whether it is a checklist, the text and every line.
std::string contentKey(const Note& n)
{
    std::string key = n.checklist ? "1" : "0";
    key += '\0' + n.body;
    for (const auto& item : n.items) key += std::string(1, '\0') + (item.done ? "1" : "0") + item.text;
    return key;
}

std::string labelsKey(const Note& n)
{
    std::string key;
    for (const auto& label : n.labels) key += label + '\0';
    return key;
}

// True when b's version of a part should be kept: it is newer, or as new and its value larger.
template <typename Value>
bool takeB(std::int64_t stampA, std::int64_t stampB, const Value& valueA, const Value& valueB)
{
    if (stampA != stampB) return stampB > stampA;
    return valueA < valueB;
}

} // namespace

Note merge(const Note& a, const Note& b)
{
    Note m = a;
    if (takeB(a.stamps.title, b.stamps.title, a.title, b.title)) {
        m.title = b.title;
        m.stamps.title = b.stamps.title;
    }
    if (takeB(a.stamps.content, b.stamps.content, contentKey(a), contentKey(b))) {
        m.body = b.body;
        m.checklist = b.checklist;
        m.items = b.items;
        m.stamps.content = b.stamps.content;
    }
    if (takeB(a.stamps.color, b.stamps.color, a.color, b.color)) {
        m.color = b.color;
        m.stamps.color = b.stamps.color;
    }
    if (takeB(a.stamps.pinned, b.stamps.pinned, a.pinned, b.pinned)) {
        m.pinned = b.pinned;
        m.stamps.pinned = b.stamps.pinned;
    }
    if (takeB(a.stamps.archived, b.stamps.archived, a.archived, b.archived)) {
        m.archived = b.archived;
        m.stamps.archived = b.stamps.archived;
    }
    if (takeB(a.stamps.trashed, b.stamps.trashed, a.trashed, b.trashed)) {
        m.trashed = b.trashed;
        m.stamps.trashed = b.stamps.trashed;
    }
    if (takeB(a.stamps.labels, b.stamps.labels, labelsKey(a), labelsKey(b))) {
        m.labels = b.labels;
        m.stamps.labels = b.stamps.labels;
    }
    m.modified = std::max(a.modified, b.modified);
    if (a.created == 0 || (b.created != 0 && b.created < a.created)) m.created = b.created;
    return m;
}

} // namespace notemerge
