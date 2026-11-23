#include "testkit.h"

#include "common/notejson.h"

#include <string>

TEST(notejson, every_field_survives_a_round_trip)
{
    Note n;
    n.id = "0123456789abcdef0123456789abcdef";
    n.title = "Café \"quotes\" ☕";
    n.body = "line one\nline two\tকা";
    n.checklist = true;
    n.items = {{"Milk", false}, {"Bread é", true}};
    n.color = 7;
    n.pinned = true;
    n.archived = false;
    n.trashed = true;
    n.labels = {"Work", "Ideas"};
    n.created = 1764000000123;
    n.modified = 1764000000456;
    n.stamps = {1, 2, 3, 4, 5, 6, 1764000000789};
    const auto back = notejson::read(notejson::write(n));
    CHECK(back.has_value());
    CHECK(*back == n);
}

TEST(notejson, text_that_isnt_a_note_is_refused)
{
    CHECK(!notejson::read("").has_value());
    CHECK(!notejson::read("not json").has_value());
    CHECK(!notejson::read("[1,2]").has_value());
    CHECK(!notejson::read("{\"v\":1}").has_value());                    // no id
    CHECK(!notejson::read("{\"id\":\"x\"}").has_value());               // no version
}

TEST(notejson, missing_fields_take_defaults)
{
    const auto n = notejson::read("{\"v\":1,\"id\":\"x\",\"title\":\"t\"}");
    CHECK(n.has_value());
    CHECK_EQ(n->title, std::string("t"));
    CHECK(n->items.empty());
    CHECK_EQ(n->stamps.content, std::int64_t(0));
}
