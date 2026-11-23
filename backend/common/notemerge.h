#pragma once

#include "common/note.h"

// Two versions of one note (this device's and the server's) made into one, part by part: for each
// part the newer change wins. Equal times are settled by the values themselves, so both devices
// end up with the same note whichever of them merges.
namespace notemerge {

Note merge(const Note& a, const Note& b);

} // namespace notemerge
