#include "notesView/masonry.h"

#include <algorithm>
#include <cmath>

namespace masonry {

Grid gridFor(double width, double cardWidth, double gap, bool compact)
{
    Grid g;
    if (compact) {
        g.columns = 2;
        g.columnWidth = std::max(0.0, (width - gap) / 2);
    } else {
        g.columns = std::max(1, static_cast<int>(std::floor((width + gap) / (cardWidth + gap))));
        g.columnWidth = cardWidth;
    }
    const double used = g.columns * g.columnWidth + (g.columns - 1) * gap;
    g.leftEdge = std::max(0.0, (width - used) / 2);
    return g;
}

namespace {

// Places cards [from, to) starting at `top`; returns the bottom of the tallest column.
double placeGroup(const std::vector<double>& heights, int from, int to, double top, const Grid& g, double gap,
                  Layout& out)
{
    std::vector<double> columns(static_cast<std::size_t>(g.columns), top);
    for (int i = from; i < to; ++i) {
        const auto shortest = std::min_element(columns.begin(), columns.end()) - columns.begin();
        out.x[i] = g.leftEdge + static_cast<double>(shortest) * (g.columnWidth + gap);
        out.y[i] = columns[shortest];
        columns[shortest] += heights[i] + gap;
    }
    return *std::max_element(columns.begin(), columns.end()) - gap;
}

} // namespace

Layout layout(const std::vector<double>& heights, int pinnedCount, double width, double cardWidth, double gap,
              double labelHeight, bool compact)
{
    const Grid g = gridFor(width, cardWidth, gap, compact);
    const int count = static_cast<int>(heights.size());
    const int pinned = std::clamp(pinnedCount, 0, count);
    Layout out;
    out.x.assign(heights.size(), 0);
    out.y.assign(heights.size(), 0);

    double top = 0;
    if (pinned > 0) {
        out.pinnedLabelY = 0;
        top = placeGroup(heights, 0, pinned, labelHeight, g, gap, out) + 2 * gap;
    }
    if (count > pinned) {
        if (pinned > 0) {
            out.othersLabelY = top;
            top += labelHeight;
        }
        top = placeGroup(heights, pinned, count, top, g, gap, out);
    } else if (pinned > 0) {
        top -= 2 * gap;
    }
    out.height = count > 0 ? top : 0;
    return out;
}

} // namespace masonry
