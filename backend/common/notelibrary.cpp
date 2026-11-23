#include "common/notelibrary.h"

NoteLibrary::NoteLibrary(std::filesystem::path folder, const crypto::Key& key, notes::NoteList::Clock clock)
    : m_store(std::move(folder), key), m_list(std::move(clock), crypto::randomId)
{
    auto loaded = m_store.load();
    m_unreadable = loaded.unreadable;
    m_list.load(std::move(loaded.notes), loaded.labels);

    m_list.onNoteChanged = [this](const std::string& id) {
        if (const Note* n = m_list.find(id)) saved(m_store.save(*n));
        if (onEdited) onEdited(id);
    };
    m_list.onNoteErased = [this](const std::string& id) {
        saved(m_store.remove(id));
        if (onErased) onErased(id);
    };
    m_list.onLabelsChanged = [this] {
        const auto& own = m_list.ownLabels();
        saved(m_store.saveLabels({own.begin(), own.end()}));
    };
}

void NoteLibrary::applyRemote(const Note& note)
{
    RemoteChange change{true, true};
    if (const Note* before = m_list.find(note.id)) {
        change.title = before->title != note.title;
        change.content = before->body != note.body || before->checklist != note.checklist || before->items != note.items;
    }
    saved(m_store.save(note));
    m_list.put(note);
    if (onRemoteChanged && (change.title || change.content)) onRemoteChanged(note.id, change);
}

void NoteLibrary::removeRemote(const std::string& id)
{
    const bool had = m_list.find(id) != nullptr;
    saved(m_store.remove(id));
    m_list.drop(id);
    if (had && onRemoteRemoved) onRemoteRemoved(id);
}

void NoteLibrary::saved(bool ok)
{
    if (!ok) m_saveFailed = true;
}
