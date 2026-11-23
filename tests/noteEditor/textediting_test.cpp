#include "testkit.h"

#include "noteEditor/textediting.h"

#include <string>

using textediting::Action;
using textediting::onEnter;
using textediting::onEnterIn;
using textediting::scrollToShow;

TEST(textediting, numbers_count_up)
{
    const Action a = onEnter("1. Milk", "");
    CHECK(a.kind == Action::Kind::Continue);
    CHECK_EQ(a.insert, std::string("\n2. "));
    CHECK_EQ(onEnter("9) Bread", "").insert, std::string("\n10) "));
    CHECK_EQ(onEnter("41.   wide gap", "").insert, std::string("\n42.   "));
}

TEST(textediting, letters_count_up)
{
    CHECK_EQ(onEnter("a. first", "").insert, std::string("\nb. "));
    CHECK_EQ(onEnter("Z) last", "").insert, std::string("\nAA) "));
}

TEST(textediting, bullets_repeat)
{
    CHECK_EQ(onEnter("- one", "").insert, std::string("\n- "));
    CHECK_EQ(onEnter("* one", "").insert, std::string("\n* "));
    CHECK_EQ(onEnter("• one", "").insert, std::string("\n• "));
}

TEST(textediting, indent_is_kept)
{
    CHECK_EQ(onEnter("    3. nested", "").insert, std::string("\n    4. "));
    CHECK_EQ(onEnter("\t- tabbed", "").insert, std::string("\n\t- "));
}

TEST(textediting, enter_in_the_middle_still_continues)
{
    const Action a = onEnter("1. Mi", "lk");
    CHECK(a.kind == Action::Kind::Continue);
    CHECK_EQ(a.insert, std::string("\n2. "));
}

TEST(textediting, empty_item_ends_the_list)
{
    CHECK(onEnter("2. ", "").kind == Action::Kind::End);
    CHECK(onEnter("  - ", "  ").kind == Action::Kind::End);
}

TEST(textediting, plain_text_is_left_alone)
{
    CHECK(onEnter("Hello", "").kind == Action::Kind::None);
    CHECK(onEnter("1.5 litres", "").kind == Action::Kind::None);
    CHECK(onEnter("e.g. this", "").kind == Action::Kind::None);
    CHECK(onEnter("Hello.", "").kind == Action::Kind::None);
    CHECK(onEnter("1.", "").kind == Action::Kind::None);         // no space after the marker yet
    CHECK(onEnter("-dash", "").kind == Action::Kind::None);
    CHECK(onEnter("", "").kind == Action::Kind::None);
}

TEST(textediting, unusual_characters_in_the_text)
{
    CHECK_EQ(onEnter("1. café ☕ কা", "").insert, std::string("\n2. "));
}

TEST(textediting, whole_text_continues_the_cursors_line)
{
    const std::string text = "Shopping\n1. Milk\nend";
    const auto e = onEnterIn(text, 16);         // after "1. Milk"
    CHECK(e.handled);
    CHECK_EQ(e.removeFrom, std::size_t(16));
    CHECK_EQ(e.removeTo, std::size_t(16));
    CHECK_EQ(e.insert, std::string("\n2. "));
    CHECK_EQ(e.cursorAfter, std::size_t(20));                 // after the new "2. "
}

TEST(textediting, whole_text_ends_an_empty_item)
{
    const std::string text = "1. Milk\n2. ";
    const auto e = onEnterIn(text, text.size());
    CHECK(e.handled);
    CHECK_EQ(e.removeFrom, std::size_t(8));                   // the start of "2. "
    CHECK_EQ(e.removeTo, text.size());
    CHECK_EQ(e.insert, std::string());
    CHECK_EQ(e.cursorAfter, std::size_t(8));
}

TEST(textediting, whole_text_plain_lines_and_edges)
{
    CHECK(!onEnterIn("Hello", 5).handled);
    CHECK(!onEnterIn("", 0).handled);
    CHECK(onEnterIn("- a", 99).handled);         // a cursor past the end is clamped
}

TEST(textediting, a_visible_line_leaves_the_view_alone)
{
    CHECK_EQ(scrollToShow(120, 20, 100, 200), 100.0);
}

TEST(textediting, a_line_above_the_view_scrolls_up_to_it)
{
    CHECK_EQ(scrollToShow(40, 20, 100, 200), 40.0);
}

TEST(textediting, a_line_below_the_view_scrolls_until_its_bottom_shows)
{
    CHECK_EQ(scrollToShow(310, 20, 100, 200), 130.0);
}
