#pragma once

#include <vector>

// Where the cards of the grid go: columns of uneven height, each card under the shortest column
// so far. The pinned cards come first under their own label row, then the others under theirs.
namespace masonry {

struct Grid {
    int columns = 1;
    double columnWidth = 0;
    double leftEdge = 0;        // the columns are centred in the width
};

// On a phone-width window there are always two columns filling the width.
Grid gridFor(double width, double cardWidth, double gap, bool compact);

struct Layout {
    std::vector<double> x;
    std::vector<double> y;
    double pinnedLabelY = -1;   // -1: that label row isn't shown
    double othersLabelY = -1;
    double height = 0;
};

// heights: the cards in model order; the first `pinnedCount` of them are pinned.
Layout layout(const std::vector<double>& heights, int pinnedCount, double width, double cardWidth, double gap,
              double labelHeight, bool compact);

} // namespace masonry
