#include "testkit.h"

#include "common/crypto.h"

#include <string>

namespace {

const crypto::Key key = *crypto::keyFromHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
const crypto::Key otherKey = *crypto::keyFromHex("FFFEFDFCFBFAF9F8F7F6F5F4F3F2F1F0EFEEEDECEBEAE9E8E7E6E5E4E3E2E1E0");
const std::string id = "0123456789abcdef0123456789abcdef";

} // namespace

TEST(crypto, keys_are_read_from_64_hex_characters)
{
    CHECK(crypto::keyFromHex(std::string(64, 'a')).has_value());
    CHECK_EQ((*crypto::keyFromHex("0a" + std::string(62, '0')))[0], 10);
    CHECK(!crypto::keyFromHex(std::string(63, 'a')).has_value());
    CHECK(!crypto::keyFromHex(std::string(65, 'a')).has_value());
    CHECK(!crypto::keyFromHex(std::string(62, 'a') + "zz").has_value());
    CHECK(!crypto::keyFromHex("").has_value());
}

TEST(crypto, sealed_text_opens_again)
{
    const std::string text = "{\"title\":\"Groceries\",\"body\":\"café ☕ কা\\nline two\"}";
    const std::string sealed = crypto::seal(key, id, text);
    CHECK_EQ(sealed.size(), text.size() + 41);
    CHECK_EQ(static_cast<int>(sealed[0]), 1);
    CHECK(sealed.find("Groceries") == std::string::npos);
    const auto opened = crypto::open(key, id, sealed);
    CHECK(opened.has_value());
    CHECK_EQ(*opened, text);
    CHECK_EQ(*crypto::open(key, id, crypto::seal(key, id, "")), std::string());
}

TEST(crypto, every_seal_uses_a_new_nonce)
{
    CHECK(crypto::seal(key, id, "same") != crypto::seal(key, id, "same"));
}

TEST(crypto, wrong_key_fails)
{
    CHECK(!crypto::open(otherKey, id, crypto::seal(key, id, "secret")).has_value());
}

TEST(crypto, changed_bytes_fail)
{
    const std::string sealed = crypto::seal(key, id, "secret");
    for (std::size_t i = 0; i < sealed.size(); ++i) {
        std::string changed = sealed;
        changed[i] = static_cast<char>(changed[i] ^ 0x01);
        CHECK(!crypto::open(key, id, changed).has_value());
    }
    CHECK(!crypto::open(key, id, sealed.substr(0, sealed.size() - 1)).has_value());
    CHECK(!crypto::open(key, id, "short").has_value());
}

TEST(crypto, a_note_moved_to_another_id_fails)
{
    CHECK(!crypto::open(key, "ffffffffffffffffffffffffffffffff", crypto::seal(key, id, "secret")).has_value());
}

TEST(crypto, wipe_zeroes_the_key)
{
    crypto::Key k = key;
    crypto::wipe(k);
    CHECK(k == crypto::Key{});
}

TEST(crypto, random_ids_are_32_hex_characters)
{
    const std::string a = crypto::randomId();
    CHECK_EQ(a.size(), std::size_t(32));
    CHECK(a.find_first_not_of("0123456789abcdef") == std::string::npos);
    CHECK(a != crypto::randomId());
}
