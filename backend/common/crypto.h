#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// Sealing a note with the app's key (XChaCha20-Poly1305). The only code that reaches Monocypher.
// Sealed bytes: version (1) | nonce (24) | MAC (16) | encrypted text.
namespace crypto {

using Key = std::array<std::uint8_t, 32>;

// The key written as 64 hex characters (either case); nothing when the text isn't one.
std::optional<Key> keyFromHex(std::string_view hex);

// `noteId` is bound into the result: it only opens under the same id. Every call uses a new
// random nonce, so sealing the same text twice gives different bytes.
std::string seal(const Key& key, std::string_view noteId, std::string_view text);

// The text, or nothing when the key is wrong, the bytes were changed or belong to another id.
std::optional<std::string> open(const Key& key, std::string_view noteId, std::string_view sealed);

void wipe(Key& key);

// A new note id: 128 random bits as 32 lowercase hex characters.
std::string randomId();

} // namespace crypto
