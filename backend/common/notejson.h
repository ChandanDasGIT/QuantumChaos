#pragma once

#include "common/note.h"

#include <optional>
#include <string>
#include <string_view>

// A note as JSON text: what is sealed into a note file and sent to the sync server.
namespace notejson {

std::string write(const Note& note);
// Nothing when the text isn't a note.
std::optional<Note> read(std::string_view text);

} // namespace notejson
