#include "common/notestore.h"

#include "common/files.h"
#include "common/json.h"
#include "common/notejson.h"

#include <algorithm>
#include <system_error>

namespace {

const std::string labelsId = "labels";
const std::string noteSuffix = ".note";

bool isNoteId(const std::string& s)
{
    return s.size() == 32 && std::all_of(s.begin(), s.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

} // namespace

NoteStore::NoteStore(std::filesystem::path folder, const crypto::Key& key) : m_folder(std::move(folder)), m_key(key) {}

NoteStore::~NoteStore()
{
    crypto::wipe(m_key);
}

std::filesystem::path NoteStore::noteFile(const std::string& id) const
{
    return m_folder / "notes" / (id + noteSuffix);
}

std::string NoteStore::seal(const Note& note) const
{
    return crypto::seal(m_key, note.id, notejson::write(note));
}

std::optional<Note> NoteStore::open(const std::string& id, const std::string& sealed) const
{
    const auto text = crypto::open(m_key, id, sealed);
    if (!text) return std::nullopt;
    auto note = notejson::read(*text);
    if (!note || note->id != id) return std::nullopt;
    return note;
}

NoteStore::Loaded NoteStore::load() const
{
    Loaded loaded;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(m_folder / "notes", ec)) {
        const std::filesystem::path& file = entry.path();
        if (file.extension() != noteSuffix) continue;
        const std::string id = file.stem().string();
        const auto bytes = files::read(file);
        const auto note = isNoteId(id) && bytes ? open(id, *bytes) : std::nullopt;
        if (note)
            loaded.notes.push_back(*note);
        else
            ++loaded.unreadable;
    }
    if (const auto bytes = files::read(m_folder / ("labels" + noteSuffix))) {
        const auto text = crypto::open(m_key, labelsId, *bytes);
        const auto parsed = text ? json::parse(*text) : json::ParseResult{};
        if (text && parsed.ok()) {
            for (const auto& label : parsed.value.asArray())
                if (label.isString()) loaded.labels.push_back(label.asString());
        } else {
            ++loaded.unreadable;
        }
    }
    return loaded;
}

bool NoteStore::save(const Note& note)
{
    if (!isNoteId(note.id)) return false;
    return files::write(noteFile(note.id), seal(note));
}

bool NoteStore::remove(const std::string& id)
{
    if (!isNoteId(id)) return false;
    std::error_code ec;
    std::filesystem::remove(noteFile(id), ec);
    return !ec;
}

bool NoteStore::saveLabels(const std::vector<std::string>& labels)
{
    json::Value list = json::Value::array();
    for (const auto& label : labels) list.push(label);
    return files::write(m_folder / ("labels" + noteSuffix), crypto::seal(m_key, labelsId, json::write(list)));
}
