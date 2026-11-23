#pragma once

#include "common/crypto.h"
#include "common/note.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// The notes kept on this device, encrypted with the app key:
//     <folder>/notes/<id>.note    one note: its JSON, sealed
//     <folder>/labels.note        the labels no note has yet
// A file that can't be opened (another key, damaged) is counted and left untouched.
class NoteStore {
public:
    NoteStore(std::filesystem::path folder, const crypto::Key& key);
    ~NoteStore();
    NoteStore(const NoteStore&) = delete;
    NoteStore& operator=(const NoteStore&) = delete;

    struct Loaded {
        std::vector<Note> notes;
        std::vector<std::string> labels;
        int unreadable = 0;
    };
    Loaded load() const;

    bool save(const Note& note);
    bool remove(const std::string& id);
    bool saveLabels(const std::vector<std::string>& labels);

    // What a note file holds: the note sealed with the key (the same bytes the server keeps).
    std::string seal(const Note& note) const;
    // Nothing when the bytes don't open with the key under `id`, or aren't note `id`.
    std::optional<Note> open(const std::string& id, const std::string& sealed) const;

private:
    std::filesystem::path noteFile(const std::string& id) const;

    std::filesystem::path m_folder;
    crypto::Key m_key;
};
