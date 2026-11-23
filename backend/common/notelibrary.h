#pragma once

#include "common/crypto.h"
#include "common/notelist.h"
#include "common/notestore.h"

#include <filesystem>
#include <functional>
#include <string>

// The app's notes: read from the device when the app starts, and each note saved again as soon
// as it changes (there is no save step).
class NoteLibrary {
public:
    NoteLibrary(std::filesystem::path folder, const crypto::Key& key, notes::NoteList::Clock clock);
    NoteLibrary(const NoteLibrary&) = delete;
    NoteLibrary& operator=(const NoteLibrary&) = delete;

    notes::NoteList& list() { return m_list; }
    const notes::NoteList& list() const { return m_list; }
    NoteStore& store() { return m_store; }

    // Note files that couldn't be opened at start (another key, damaged). They are left as they are.
    int unreadable() const { return m_unreadable; }
    // A change couldn't be written to the device (disk full, no permission).
    bool saveFailed() const { return m_saveFailed; }

    // A note as it came from the server: kept and saved, but not reported as an edit made here.
    void applyRemote(const Note& note);
    // A note deleted for good on another device.
    void removeRemote(const std::string& id);

    // An edit made on this device, after it is saved: note `id` changed, or was deleted for good.
    std::function<void(const std::string& id)> onEdited;
    std::function<void(const std::string& id)> onErased;

    // A change from another device reached note `id`: which of its text a screen showing it has to
    // read again (colour, pin and labels are read again anyway). Or the note was deleted there.
    struct RemoteChange {
        bool title = false;
        bool content = false;       // the text, or the checklist
    };
    std::function<void(const std::string& id, RemoteChange change)> onRemoteChanged;
    std::function<void(const std::string& id)> onRemoteRemoved;

private:
    void saved(bool ok);

    NoteStore m_store;
    notes::NoteList m_list;
    int m_unreadable = 0;
    bool m_saveFailed = false;
};
