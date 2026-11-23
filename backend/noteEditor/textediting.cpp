#include "noteEditor/textediting.h"

#include <algorithm>
#include <cctype>

namespace textediting {

namespace {

enum class ListKind { Number, Lower, Upper, Bullet };

struct ListMarker {
    std::size_t indentLen = 0;
    ListKind kind = ListKind::Number;
    unsigned long long value = 1;   // the number, or the letter position (a = 1, z = 26, aa = 27)
    std::string symbol;             // the delimiter ('.' or ')'), or the bullet itself
    std::string gap;                // spaces after the delimiter or bullet
    std::size_t end = 0;            // where the text after the marker starts
};

std::size_t indentOf(const std::string& line)
{
    std::size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    return i;
}

std::string lettersOf(unsigned long long v, bool upper)
{
    std::string s;
    while (v > 0) {
        --v;
        s.insert(s.begin(), static_cast<char>((upper ? 'A' : 'a') + static_cast<int>(v % 26)));
        v /= 26;
    }
    return s;
}

std::string formatMarker(const ListMarker& m, unsigned long long value)
{
    if (m.kind == ListKind::Bullet) return m.symbol + m.gap;
    const std::string t = m.kind == ListKind::Number ? std::to_string(value) : lettersOf(value, m.kind == ListKind::Upper);
    return t + m.symbol + m.gap;
}

std::size_t gapAfter(const std::string& line, std::size_t from)
{
    std::size_t g = from;
    while (g < line.size() && line[g] == ' ') ++g;
    return g;
}

// Recognises "  12. text", "3) text", "b. text", "AA) text", "- text", "* text", "• text".
// Needs at least one space after the marker.
bool listMarkerOf(const std::string& line, ListMarker& m)
{
    const std::size_t i = indentOf(line);
    for (const std::string bullet : {"- ", "* ", "• "}) {
        const std::string symbol = bullet.substr(0, bullet.size() - 1);
        if (line.compare(i, symbol.size(), symbol) == 0 && i + symbol.size() < line.size() &&
            line[i + symbol.size()] == ' ') {
            const std::size_t g = gapAfter(line, i + symbol.size());
            m = {i, ListKind::Bullet, 0, symbol, line.substr(i + symbol.size(), g - i - symbol.size()), g};
            return true;
        }
    }

    std::size_t d = i;
    ListKind kind = ListKind::Number;
    unsigned long long value = 0;
    if (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i]))) {
        while (d < line.size() && std::isdigit(static_cast<unsigned char>(line[d]))) ++d;
        if (d - i > 9) return false;
        value = std::stoull(line.substr(i, d - i));
    } else if (i < line.size() && std::isalpha(static_cast<unsigned char>(line[i]))) {
        const bool upper = std::isupper(static_cast<unsigned char>(line[i])) != 0;
        while (d < line.size() && std::isalpha(static_cast<unsigned char>(line[d])) &&
               (std::isupper(static_cast<unsigned char>(line[d])) != 0) == upper)
            ++d;
        if (d - i > 3) return false;                           // longer words are text, not markers
        kind = upper ? ListKind::Upper : ListKind::Lower;
        for (std::size_t k = i; k < d; ++k)
            value = value * 26 + static_cast<unsigned long long>(std::tolower(static_cast<unsigned char>(line[k])) - 'a' + 1);
    } else {
        return false;
    }

    if (d >= line.size() || (line[d] != '.' && line[d] != ')')) return false;
    const std::size_t g = gapAfter(line, d + 1);
    if (g == d + 1) return false;
    m = {i, kind, value, std::string(1, line[d]), line.substr(d + 1, g - d - 1), g};
    return true;
}

bool blank(const std::string& s)
{
    for (unsigned char c : s)
        if (!std::isspace(c)) return false;
    return true;
}

} // namespace

Action onEnter(const std::string& lineBefore, const std::string& lineAfter)
{
    ListMarker m;
    if (!listMarkerOf(lineBefore, m)) return {};
    if (blank(lineBefore.substr(m.end)) && blank(lineAfter)) return {Action::Kind::End, {}};
    return {Action::Kind::Continue, "\n" + lineBefore.substr(0, m.indentLen) + formatMarker(m, m.value + 1)};
}

Edit onEnterIn(const std::string& text, std::size_t cursor)
{
    cursor = std::min(cursor, text.size());
    const std::size_t newline = cursor == 0 ? std::string::npos : text.rfind('\n', cursor - 1);
    const std::size_t lineStart = newline == std::string::npos ? 0 : newline + 1;
    std::size_t lineEnd = text.find('\n', cursor);
    if (lineEnd == std::string::npos) lineEnd = text.size();

    const Action action = onEnter(text.substr(lineStart, cursor - lineStart), text.substr(cursor, lineEnd - cursor));
    switch (action.kind) {
    case Action::Kind::Continue: return {true, cursor, cursor, action.insert, cursor + action.insert.size()};
    case Action::Kind::End: return {true, lineStart, cursor, {}, lineStart};
    case Action::Kind::None: break;
    }
    return {};
}

double scrollToShow(double top, double height, double viewTop, double viewHeight)
{
    if (top < viewTop) return top;
    if (top + height > viewTop + viewHeight) return top + height - viewHeight;
    return viewTop;
}

} // namespace textediting
