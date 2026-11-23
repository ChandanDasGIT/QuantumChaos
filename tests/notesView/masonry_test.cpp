#include "testkit.h"

#include "notesView/masonry.h"

#include <vector>

TEST(masonry, columns_fit_the_width_and_are_centred)
{
    const auto g = masonry::gridFor(990, 240, 12, false);
    CHECK_EQ(g.columns, 3);                                   // 4 would need 996
    CHECK_EQ(g.columnWidth, 240.0);
    CHECK_EQ(g.leftEdge, (990 - (3 * 240 + 2 * 12)) / 2.0);
    CHECK_EQ(masonry::gridFor(100, 240, 12, false).columns, 1);
}

TEST(masonry, phone_width_has_two_full_columns)
{
    const auto g = masonry::gridFor(376, 240, 12, true);
    CHECK_EQ(g.columns, 2);
    CHECK_EQ(g.columnWidth, 182.0);
    CHECK_EQ(g.leftEdge, 0.0);
}

TEST(masonry, each_card_goes_under_the_shortest_column)
{
    // 2 columns of 100 at width 212, gap 12: cards 50, 200, 30 -> the third goes under the first.
    const auto l = masonry::layout({50, 200, 30}, 0, 212, 100, 12, 20, false);
    CHECK_EQ(l.x[0], 0.0);
    CHECK_EQ(l.x[1], 112.0);
    CHECK_EQ(l.x[2], 0.0);
    CHECK_EQ(l.y[2], 62.0);
    CHECK_EQ(l.height, 200.0);
    CHECK_EQ(l.pinnedLabelY, -1.0);
    CHECK_EQ(l.othersLabelY, -1.0);
}

TEST(masonry, pinned_cards_come_first_under_their_own_labels)
{
    const auto l = masonry::layout({50, 80}, 1, 100, 100, 10, 20, false);
    CHECK_EQ(l.pinnedLabelY, 0.0);
    CHECK_EQ(l.y[0], 20.0);                                   // under the PINNED row
    CHECK_EQ(l.othersLabelY, 90.0);                           // 20 + 50, then two gaps
    CHECK_EQ(l.y[1], 110.0);                                  // under the OTHERS row
    CHECK_EQ(l.height, 190.0);
}

TEST(masonry, only_pinned_or_nothing)
{
    const auto pinnedOnly = masonry::layout({50}, 1, 100, 100, 10, 20, false);
    CHECK_EQ(pinnedOnly.othersLabelY, -1.0);
    CHECK_EQ(pinnedOnly.height, 70.0);
    CHECK_EQ(masonry::layout({}, 0, 100, 100, 10, 20, false).height, 0.0);
}
