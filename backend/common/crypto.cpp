#include "common/crypto.h"

#include "monocypher.h"

#include <random>

namespace crypto {

namespace {

constexpr std::uint8_t formatVersion = 1;
constexpr std::size_t nonceSize = 24;
constexpr std::size_t macSize = 16;
constexpr std::size_t headerSize = 1 + nonceSize + macSize;

const std::uint8_t* bytes(std::string_view s)
{
    return reinterpret_cast<const std::uint8_t*>(s.data());
}

std::uint8_t* bytes(std::string& s)
{
    return reinterpret_cast<std::uint8_t*>(s.data());
}

int hexDigit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// The system's secure random source (getrandom / urandom on Linux and Android, the Windows
// crypto provider with MinGW).
void randomFill(std::uint8_t* out, std::size_t size)
{
    static std::random_device source;
    for (std::size_t i = 0; i < size; i += 4) {
        const auto r = source();
        for (std::size_t k = 0; k < 4 && i + k < size; ++k) out[i + k] = static_cast<std::uint8_t>(r >> (8 * k));
    }
}

} // namespace

std::optional<Key> keyFromHex(std::string_view hex)
{
    Key key{};
    if (hex.size() != key.size() * 2) return std::nullopt;
    for (std::size_t i = 0; i < key.size(); ++i) {
        const int high = hexDigit(hex[2 * i]);
        const int low = hexDigit(hex[2 * i + 1]);
        if (high < 0 || low < 0) return std::nullopt;
        key[i] = static_cast<std::uint8_t>(high * 16 + low);
    }
    return key;
}

std::string seal(const Key& key, std::string_view noteId, std::string_view text)
{
    std::string out(headerSize + text.size(), '\0');
    std::uint8_t* p = bytes(out);
    p[0] = formatVersion;
    std::uint8_t* nonce = p + 1;
    std::uint8_t* mac = nonce + nonceSize;
    randomFill(nonce, nonceSize);
    crypto_aead_lock(p + headerSize, mac, key.data(), nonce, bytes(noteId), noteId.size(), bytes(text), text.size());
    return out;
}

std::optional<std::string> open(const Key& key, std::string_view noteId, std::string_view sealed)
{
    if (sealed.size() < headerSize || static_cast<std::uint8_t>(sealed[0]) != formatVersion) return std::nullopt;
    const std::uint8_t* nonce = bytes(sealed) + 1;
    const std::uint8_t* mac = nonce + nonceSize;
    std::string text(sealed.size() - headerSize, '\0');
    if (crypto_aead_unlock(bytes(text), mac, key.data(), nonce, bytes(noteId), noteId.size(),
                           bytes(sealed) + headerSize, text.size()) != 0) {
        crypto_wipe(text.data(), text.size());
        return std::nullopt;
    }
    return text;
}

void wipe(Key& key)
{
    crypto_wipe(key.data(), key.size());
}

std::string randomId()
{
    std::array<std::uint8_t, 16> bits{};
    randomFill(bits.data(), bits.size());
    static const char* digits = "0123456789abcdef";
    std::string id;
    for (const std::uint8_t b : bits) {
        id.push_back(digits[b >> 4]);
        id.push_back(digits[b & 15]);
    }
    return id;
}

} // namespace crypto
