#include "common/json.h"

#include <cctype>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace json {

namespace {

const Value& nullValue()
{
    static const Value value;
    return value;
}

void appendUtf8(std::string& out, std::uint32_t cp)
{
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

class Parser {
public:
    explicit Parser(std::string_view text) : m_text(text) {}

    ParseResult run()
    {
        ParseResult result;
        skipSpace();
        if (!parseValue(result.value, 0)) {
            result.value = Value();
            result.error = m_error;
            return result;
        }
        skipSpace();
        if (m_pos != m_text.size()) {
            result.value = Value();
            result.error = where() + "unexpected text after the value";
        }
        return result;
    }

private:
    static constexpr int maxDepth = 256;

    std::string where() const
    {
        int line = 1;
        for (std::size_t i = 0; i < m_pos && i < m_text.size(); ++i)
            if (m_text[i] == '\n') ++line;
        return "line " + std::to_string(line) + ": ";
    }

    bool fail(const std::string& message)
    {
        if (m_error.empty()) m_error = where() + message;
        return false;
    }

    void skipSpace()
    {
        while (m_pos < m_text.size()) {
            const char c = m_text[m_pos];
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') break;
            ++m_pos;
        }
    }

    bool literal(std::string_view word)
    {
        if (m_text.substr(m_pos, word.size()) != word) return false;
        m_pos += word.size();
        return true;
    }

    bool parseValue(Value& out, int depth)
    {
        if (depth > maxDepth) return fail("nested too deeply");
        if (m_pos >= m_text.size()) return fail("unexpected end of text");
        switch (m_text[m_pos]) {
        case '{': return parseObject(out, depth);
        case '[': return parseArray(out, depth);
        case '"': {
            std::string s;
            if (!parseString(s)) return false;
            out = Value(std::move(s));
            return true;
        }
        case 't':
            if (literal("true")) { out = Value(true); return true; }
            return fail("unknown word");
        case 'f':
            if (literal("false")) { out = Value(false); return true; }
            return fail("unknown word");
        case 'n':
            if (literal("null")) { out = Value(); return true; }
            return fail("unknown word");
        default:
            return parseNumber(out);
        }
    }

    bool parseNumber(Value& out)
    {
        const std::size_t start = m_pos;
        if (m_pos < m_text.size() && m_text[m_pos] == '-') ++m_pos;
        auto digits = [&] {
            const std::size_t from = m_pos;
            while (m_pos < m_text.size() && m_text[m_pos] >= '0' && m_text[m_pos] <= '9') ++m_pos;
            return m_pos > from;
        };
        if (!digits()) return fail("expected a value");
        if (m_pos < m_text.size() && m_text[m_pos] == '.') {
            ++m_pos;
            if (!digits()) return fail("expected digits after '.'");
        }
        if (m_pos < m_text.size() && (m_text[m_pos] == 'e' || m_text[m_pos] == 'E')) {
            ++m_pos;
            if (m_pos < m_text.size() && (m_text[m_pos] == '+' || m_text[m_pos] == '-')) ++m_pos;
            if (!digits()) return fail("expected digits in the exponent");
        }
        double value = 0.0;
        const std::string_view number = m_text.substr(start, m_pos - start);
        const std::errc ec = parseDouble(number, value);
        if (ec == std::errc::result_out_of_range) value = (number.front() == '-') ? -HUGE_VAL : HUGE_VAL;
        else if (ec != std::errc()) return fail("bad number");
        out = Value(value);
        return true;
    }

    bool hex4(std::uint32_t& out)
    {
        if (m_pos + 4 > m_text.size()) return fail("short \\u escape");
        out = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = m_text[m_pos++];
            out <<= 4;
            if (c >= '0' && c <= '9') out |= static_cast<std::uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') out |= static_cast<std::uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') out |= static_cast<std::uint32_t>(c - 'A' + 10);
            else return fail("bad \\u escape");
        }
        return true;
    }

    bool parseString(std::string& out)
    {
        ++m_pos;                                   // the opening quote
        while (true) {
            if (m_pos >= m_text.size()) return fail("unterminated string");
            const char c = m_text[m_pos++];
            if (c == '"') return true;
            if (static_cast<unsigned char>(c) < 0x20) return fail("control character in string");
            if (c != '\\') {
                out += c;
                continue;
            }
            if (m_pos >= m_text.size()) return fail("unterminated string");
            const char e = m_text[m_pos++];
            switch (e) {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case '/': out += '/'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            case 'u': {
                std::uint32_t cp = 0;
                if (!hex4(cp)) return false;
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    std::uint32_t low = 0;
                    if (m_text.substr(m_pos, 2) == "\\u") {
                        m_pos += 2;
                        if (!hex4(low)) return false;
                        if (low >= 0xDC00 && low <= 0xDFFF) cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                        else { appendUtf8(out, 0xFFFD); cp = low; }
                    } else {
                        cp = 0xFFFD;                       // a lone high surrogate
                    }
                } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                    cp = 0xFFFD;                           // a lone low surrogate
                }
                appendUtf8(out, cp);
                break;
            }
            default:
                return fail("bad escape");
            }
        }
    }

    bool parseArray(Value& out, int depth)
    {
        ++m_pos;
        Value::Array items;
        skipSpace();
        if (m_pos < m_text.size() && m_text[m_pos] == ']') {
            ++m_pos;
            out = Value(std::move(items));
            return true;
        }
        while (true) {
            skipSpace();
            Value item;
            if (!parseValue(item, depth + 1)) return false;
            items.push_back(std::move(item));
            skipSpace();
            if (m_pos >= m_text.size()) return fail("unterminated array");
            const char c = m_text[m_pos++];
            if (c == ']') break;
            if (c != ',') return fail("expected ',' or ']'");
        }
        out = Value(std::move(items));
        return true;
    }

    bool parseObject(Value& out, int depth)
    {
        ++m_pos;
        Value::Object members;
        skipSpace();
        if (m_pos < m_text.size() && m_text[m_pos] == '}') {
            ++m_pos;
            out = Value(std::move(members));
            return true;
        }
        while (true) {
            skipSpace();
            if (m_pos >= m_text.size() || m_text[m_pos] != '"') return fail("expected a key in quotes");
            std::string key;
            if (!parseString(key)) return false;
            skipSpace();
            if (m_pos >= m_text.size() || m_text[m_pos] != ':') return fail("expected ':'");
            ++m_pos;
            skipSpace();
            Value value;
            if (!parseValue(value, depth + 1)) return false;
            bool replaced = false;
            for (auto& member : members)
                if (member.first == key) { member.second = std::move(value); replaced = true; break; }
            if (!replaced) members.emplace_back(std::move(key), std::move(value));
            skipSpace();
            if (m_pos >= m_text.size()) return fail("unterminated object");
            const char c = m_text[m_pos++];
            if (c == '}') break;
            if (c != ',') return fail("expected ',' or '}'");
        }
        out = Value(std::move(members));
        return true;
    }

    std::string_view m_text;
    std::size_t m_pos = 0;
    std::string m_error;
};

void writeString(std::string& out, const std::string& s)
{
    out += '"';
    for (const char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof buf, "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
                out += buf;
            } else {
                out += c;
            }
        }
    }
    out += '"';
}

void writeNumber(std::string& out, double n)
{
    if (!std::isfinite(n)) {
        out += "null";                             // JSON has no NaN or infinity
        return;
    }
    char buf[32];
    if (n == std::floor(n) && std::fabs(n) < 1e15) {
        std::snprintf(buf, sizeof buf, "%.0f", n);
    } else {
        const auto [ptr, ec] = std::to_chars(buf, buf + sizeof buf, n);
        *ptr = '\0';
        (void)ec;
    }
    out += buf;
}

void writeValue(std::string& out, const Value& v, bool pretty, int indent)
{
    auto newline = [&](int level) {
        if (!pretty) return;
        out += '\n';
        out.append(static_cast<std::size_t>(level) * 2, ' ');
    };
    switch (v.type()) {
    case Value::Type::Null: out += "null"; break;
    case Value::Type::Bool: out += v.asBool() ? "true" : "false"; break;
    case Value::Type::Number: writeNumber(out, v.asNumber()); break;
    case Value::Type::String: writeString(out, v.asString()); break;
    case Value::Type::Array: {
        const auto& items = v.asArray();
        out += '[';
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i > 0) out += ',';
            newline(indent + 1);
            writeValue(out, items[i], pretty, indent + 1);
        }
        if (!items.empty()) newline(indent);
        out += ']';
        break;
    }
    case Value::Type::Object: {
        const auto& members = v.asObject();
        out += '{';
        for (std::size_t i = 0; i < members.size(); ++i) {
            if (i > 0) out += ',';
            newline(indent + 1);
            writeString(out, members[i].first);
            out += pretty ? ": " : ":";
            writeValue(out, members[i].second, pretty, indent + 1);
        }
        if (!members.empty()) newline(indent);
        out += '}';
        break;
    }
    }
}

} // namespace

bool Value::asBool(bool fallback) const
{
    const bool* b = std::get_if<bool>(&m_data);
    return b ? *b : fallback;
}

double Value::asNumber(double fallback) const
{
    const double* n = std::get_if<double>(&m_data);
    return n ? *n : fallback;
}

std::int64_t Value::asInt(std::int64_t fallback) const
{
    const double* n = std::get_if<double>(&m_data);
    if (!n || !std::isfinite(*n)) return fallback;
    return static_cast<std::int64_t>(std::llround(*n));
}

std::string Value::asString(std::string_view fallback) const
{
    const std::string* s = std::get_if<std::string>(&m_data);
    return s ? *s : std::string(fallback);
}

const Value::Array& Value::asArray() const
{
    static const Array empty;
    const Array* a = std::get_if<Array>(&m_data);
    return a ? *a : empty;
}

const Value::Object& Value::asObject() const
{
    static const Object empty;
    const Object* o = std::get_if<Object>(&m_data);
    return o ? *o : empty;
}

const Value& Value::operator[](std::string_view key) const
{
    for (const auto& member : asObject())
        if (member.first == key) return member.second;
    return nullValue();
}

const Value& Value::operator[](std::size_t index) const
{
    const auto& items = asArray();
    return index < items.size() ? items[index] : nullValue();
}

bool Value::contains(std::string_view key) const
{
    for (const auto& member : asObject())
        if (member.first == key) return true;
    return false;
}

std::size_t Value::size() const
{
    if (const Array* a = std::get_if<Array>(&m_data)) return a->size();
    if (const Object* o = std::get_if<Object>(&m_data)) return o->size();
    return 0;
}

void Value::set(std::string_view key, Value value)
{
    if (isNull()) m_data = Object{};
    Object* o = std::get_if<Object>(&m_data);
    if (!o) return;
    for (auto& member : *o)
        if (member.first == key) { member.second = std::move(value); return; }
    o->emplace_back(std::string(key), std::move(value));
}

void Value::push(Value value)
{
    if (isNull()) m_data = Array{};
    if (Array* a = std::get_if<Array>(&m_data)) a->push_back(std::move(value));
}

ParseResult parse(std::string_view text)
{
    return Parser(text).run();
}

std::string write(const Value& value, bool pretty)
{
    std::string out;
    writeValue(out, value, pretty, 0);
    return out;
}

std::errc parseDouble(std::string_view text, double& value)
{
#ifdef __ANDROID__
    // strtod reads '.' as the decimal point here: Android's C library has only the "C" locale.
    // It also takes what from_chars refuses, so those are turned away first.
    std::string_view rest = text;
    if (!rest.empty() && rest.front() == '-') rest.remove_prefix(1);
    if (rest.empty() || rest.front() == '+' || std::isspace(static_cast<unsigned char>(rest.front()))
        || (rest.size() > 1 && rest[0] == '0' && (rest[1] == 'x' || rest[1] == 'X')))
        return std::errc::invalid_argument;
    const std::string copy(text);
    char* end = nullptr;
    errno = 0;
    const double parsed = std::strtod(copy.c_str(), &end);
    if (end != copy.c_str() + copy.size()) return std::errc::invalid_argument;
    if (errno == ERANGE) return std::errc::result_out_of_range;
    value = parsed;
    return std::errc();
#else
    const char* last = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(text.data(), last, value);
    if (ec != std::errc()) return ec;
    return ptr == last ? std::errc() : std::errc::invalid_argument;
#endif
}

} // namespace json
