#include "testkit.h"

#include "common/json.h"

TEST(json, parses_scalars)
{
    CHECK(json::parse("null").value.isNull());
    CHECK_EQ(json::parse("true").value.asBool(), true);
    CHECK_EQ(json::parse(" false ").value.asBool(true), false);
    CHECK_EQ(json::parse("-12.5e1").value.asNumber(), -125.0);
    CHECK_EQ(json::parse("0").value.asInt(), std::int64_t(0));
    CHECK_EQ(json::parse("\"hi\"").value.asString(), std::string("hi"));
}

TEST(json, parses_nested_objects_and_arrays)
{
    const auto r = json::parse(R"({"data":[{"id":"a","n":1},{"id":"b","n":2}],"total":2,"meta":{}})");
    CHECK(r.ok());
    CHECK_EQ(r.value["data"].size(), std::size_t(2));
    CHECK_EQ(r.value["data"][1]["id"].asString(), std::string("b"));
    CHECK_EQ(r.value["total"].asInt(), std::int64_t(2));
    CHECK(r.value["meta"].isObject());
    CHECK(r.value["missing"]["deeper"][3].isNull());
}

TEST(json, decodes_escapes_and_unicode)
{
    const auto r = json::parse(R"("a\"b\\c\/d\néあ😀")");
    CHECK(r.ok());
    CHECK_EQ(r.value.asString(), std::string("a\"b\\c/d\n\xC3\xA9\xE3\x81\x82\xF0\x9F\x98\x80"));
}

TEST(json, lone_surrogate_becomes_replacement_character)
{
    CHECK_EQ(json::parse(R"("\ud83d")").value.asString(), std::string("\xEF\xBF\xBD"));
}

TEST(json, reports_errors_with_line)
{
    const auto r = json::parse("{\n\"a\" 1}");
    CHECK(!r.ok());
    CHECK(r.error.find("line 2") == 0);
    CHECK(!json::parse("[1,2").ok());
    CHECK(!json::parse("{\"a\":}").ok());
    CHECK(!json::parse("tru").ok());
    CHECK(!json::parse("1 2").ok());
    CHECK(!json::parse("\"a\nb\"").ok());
    CHECK(!json::parse("").ok());
    CHECK(!json::parse("01x").ok());
}

TEST(json, rejects_very_deep_nesting)
{
    CHECK(!json::parse(std::string(1000, '[') + std::string(1000, ']')).ok());
    CHECK(json::parse(std::string(100, '[') + std::string(100, ']')).ok());
}

TEST(json, writes_and_reads_back)
{
    json::Value v;
    v.set("title", "Café \"quoted\"\n");
    v.set("count", 3);
    v.set("big", std::int64_t(1732233600000));
    v.set("ratio", 0.25);
    v.set("on", true);
    v.set("none", nullptr);
    json::Value list;
    list.push(1);
    list.push("two");
    v.set("list", list);
    v.set("count", 4);                                   // replaces, keeps the order

    const std::string compact = json::write(v);
    CHECK_EQ(compact, std::string(R"({"title":"Café \"quoted\"\n","count":4,"big":1732233600000,"ratio":0.25,"on":true,"none":null,"list":[1,"two"]})"));
    CHECK(json::parse(compact).value == v);
    CHECK(json::parse(json::write(v, true)).value == v);
}

TEST(json, wrong_types_give_fallbacks)
{
    const auto v = json::parse(R"({"n":"5","s":5})").value;
    CHECK_EQ(v["n"].asNumber(7), 7.0);
    CHECK_EQ(v["s"].asString("x"), std::string("x"));
    CHECK(v["s"].asArray().empty());
    CHECK_EQ(v["s"].size(), std::size_t(0));
}
