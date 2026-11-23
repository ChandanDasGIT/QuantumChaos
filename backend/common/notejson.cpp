#include "common/notejson.h"

#include "common/json.h"

namespace notejson {

namespace {

constexpr int formatVersion = 1;

} // namespace

std::string write(const Note& note)
{
    json::Value v = json::Value::object();
    v.set("v", formatVersion);
    v.set("id", note.id);
    v.set("title", note.title);
    v.set("body", note.body);
    v.set("checklist", note.checklist);
    json::Value items = json::Value::array();
    for (const auto& item : note.items) {
        json::Value i = json::Value::object();
        i.set("text", item.text);
        i.set("done", item.done);
        items.push(std::move(i));
    }
    v.set("items", std::move(items));
    v.set("color", note.color);
    v.set("pinned", note.pinned);
    v.set("archived", note.archived);
    v.set("trashed", note.trashed);
    json::Value labels = json::Value::array();
    for (const auto& label : note.labels) labels.push(label);
    v.set("labels", std::move(labels));
    v.set("created", note.created);
    v.set("modified", note.modified);
    json::Value stamps = json::Value::object();
    stamps.set("title", note.stamps.title);
    stamps.set("content", note.stamps.content);
    stamps.set("color", note.stamps.color);
    stamps.set("pinned", note.stamps.pinned);
    stamps.set("archived", note.stamps.archived);
    stamps.set("trashed", note.stamps.trashed);
    stamps.set("labels", note.stamps.labels);
    v.set("stamps", std::move(stamps));
    return json::write(v);
}

std::optional<Note> read(std::string_view text)
{
    const auto parsed = json::parse(text);
    const json::Value& v = parsed.value;
    if (!parsed.ok() || !v.isObject() || !v["id"].isString() || v["v"].asInt() < 1) return std::nullopt;
    Note n;
    n.id = v["id"].asString();
    n.title = v["title"].asString();
    n.body = v["body"].asString();
    n.checklist = v["checklist"].asBool();
    for (const auto& item : v["items"].asArray()) n.items.push_back({item["text"].asString(), item["done"].asBool()});
    n.color = static_cast<int>(v["color"].asInt());
    n.pinned = v["pinned"].asBool();
    n.archived = v["archived"].asBool();
    n.trashed = v["trashed"].asBool();
    for (const auto& label : v["labels"].asArray())
        if (label.isString()) n.labels.push_back(label.asString());
    n.created = v["created"].asInt();
    n.modified = v["modified"].asInt();
    const json::Value& s = v["stamps"];
    n.stamps = {s["title"].asInt(), s["content"].asInt(), s["color"].asInt(), s["pinned"].asInt(),
                s["archived"].asInt(), s["trashed"].asInt(), s["labels"].asInt()};
    return n;
}

} // namespace notejson
