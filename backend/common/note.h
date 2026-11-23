#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

struct ChecklistItem {
    std::string text;
    bool done = false;
    bool operator==(const ChecklistItem&) const = default;
};

struct Note {
    // Colour 0 is "no colour"; 1 .. noteColorCount - 1 are the note colours of the theme.
    static constexpr int noteColorCount = 12;

    std::string id;                     // 32 lowercase hex characters
    std::string title;
    std::string body;                   // the text of a text note
    bool checklist = false;             // a checklist note keeps its lines in `items` instead of `body`
    std::vector<ChecklistItem> items;
    int color = 0;                      // index into the theme's note colours; 0 = no colour
    bool pinned = false;
    bool archived = false;
    bool trashed = false;
    std::vector<std::string> labels;
    std::int64_t created = 0;           // ms since 1970
    std::int64_t modified = 0;          // last change to the title, text or checklist

    // When each part last changed (ms since 1970), so edits made on two devices merge part by
    // part. `content` covers the text, the checklist and whether the note is a checklist.
    struct Stamps {
        std::int64_t title = 0;
        std::int64_t content = 0;
        std::int64_t color = 0;
        std::int64_t pinned = 0;
        std::int64_t archived = 0;
        std::int64_t trashed = 0;
        std::int64_t labels = 0;
        bool operator==(const Stamps&) const = default;
    };
    Stamps stamps;

    bool operator==(const Note&) const = default;

    // Nothing written in it (labels and colour don't count): such a note is dropped when closed.
    bool isEmpty() const
    {
        const auto blank = [](const std::string& s) {
            return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c) != 0; });
        };
        return blank(title) && blank(body) &&
               std::all_of(items.begin(), items.end(), [&](const ChecklistItem& i) { return blank(i.text); });
    }

    // A note in the Trash can only be restored or deleted.
    bool editable() const { return !trashed; }

    bool coloured() const { return color != 0; }

    int doneCount() const
    {
        return static_cast<int>(std::count_if(items.begin(), items.end(), [](const ChecklistItem& i) { return i.done; }));
    }

    // How many open checklist lines a card in the grid shows.
    static constexpr std::size_t cardPreviewLines = 8;

    // The first `max` lines not ticked yet.
    std::vector<ChecklistItem> openItems(std::size_t max) const
    {
        std::vector<ChecklistItem> open;
        for (const auto& item : items) {
            if (open.size() == max) break;
            if (!item.done) open.push_back(item);
        }
        return open;
    }
};
