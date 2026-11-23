#pragma once

#include <cstddef>
#include <string>

// How typing in a note's text behaves: Enter in a list item ("1. ", "b) ", "- ", "• "), and
// keeping the line with the cursor in view.
namespace textediting {

struct Action {
    enum class Kind {
        None,       // not a list item: a plain new line
        Continue,   // insert `insert` at the cursor: a new line with the next marker
        End,        // the item is empty: remove the line's marker (everything before the cursor)
    };
    Kind kind = Kind::None;
    std::string insert;
};

// lineBefore: the cursor's line up to the cursor. lineAfter: the rest of that line.
Action onEnter(const std::string& lineBefore, const std::string& lineAfter);

// Enter pressed in the whole text at byte `cursor`. When handled, the editor replaces bytes
// [removeFrom, removeTo) with `insert` instead of adding a plain new line.
struct Edit {
    bool handled = false;
    std::size_t removeFrom = 0;
    std::size_t removeTo = 0;
    std::string insert;
    std::size_t cursorAfter = 0;    // where the cursor goes in the edited text
};
Edit onEnterIn(const std::string& text, std::size_t cursor);

// Where to scroll so the line with the cursor stays in view: the view shows
// [viewTop, viewTop + viewHeight] and the line is [top, top + height]. Unchanged if already visible.
double scrollToShow(double top, double height, double viewTop, double viewHeight);

} // namespace textediting
