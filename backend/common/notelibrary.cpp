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
    saved(m_store.save(note));
    m_list.put(note);
}

void NoteLibrary::removeRemote(const std::string& id)
{
    saved(m_store.remove(id));
    m_list.drop(id);
}

void NoteLibrary::saved(bool ok)
{
    if (!ok) m_saveFailed = true;
}
