#pragma once

#include <cstdint>
#include <string>
#include <system_error>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

// JSON values, a parser and a writer. Objects keep their keys in the order they were read or set.
namespace json {

class Value {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };
    using Array = std::vector<Value>;
    using Object = std::vector<std::pair<std::string, Value>>;

    Value() = default;
    Value(std::nullptr_t) {}
    Value(bool b) : m_data(b) {}
    Value(double n) : m_data(n) {}
    Value(int n) : m_data(static_cast<double>(n)) {}
    Value(std::int64_t n) : m_data(static_cast<double>(n)) {}
    Value(const char* s) : m_data(std::string(s)) {}
    Value(std::string s) : m_data(std::move(s)) {}
    Value(std::string_view s) : m_data(std::string(s)) {}
    Value(Array a) : m_data(std::move(a)) {}
    Value(Object o) : m_data(std::move(o)) {}

    static Value array() { return Value(Array{}); }
    static Value object() { return Value(Object{}); }

    Type type() const { return static_cast<Type>(m_data.index()); }
    bool isNull() const { return type() == Type::Null; }
    bool isBool() const { return type() == Type::Bool; }
    bool isNumber() const { return type() == Type::Number; }
    bool isString() const { return type() == Type::String; }
    bool isArray() const { return type() == Type::Array; }
    bool isObject() const { return type() == Type::Object; }

    // The value, or the fallback when it is of another type.
    bool asBool(bool fallback = false) const;
    double asNumber(double fallback = 0.0) const;
    std::int64_t asInt(std::int64_t fallback = 0) const;
    std::string asString(std::string_view fallback = {}) const;
    // Empty when not an array / object.
    const Array& asArray() const;
    const Object& asObject() const;

    // A member or element; a null value when it is missing or this is not an object / array.
    const Value& operator[](std::string_view key) const;
    const Value& operator[](std::size_t index) const;
    bool contains(std::string_view key) const;
    // Arrays and objects: the number of elements / members; 0 for anything else.
    std::size_t size() const;

    // Replaces the member `key` (or adds it); turns a null value into an object first.
    void set(std::string_view key, Value value);
    // Appends to an array; turns a null value into an array first.
    void push(Value value);

    bool operator==(const Value& other) const = default;

private:
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> m_data;
};

struct ParseResult {
    Value value;
    std::string error;          // empty on success: "line 3: expected ':'"
    bool ok() const { return error.empty(); }
};

ParseResult parse(std::string_view text);

// Compact by default; pretty: two-space indent, one member per line.
std::string write(const Value& value, bool pretty = false);

// The whole of `text` as a double, read like std::from_chars: success, result_out_of_range
// (value untouched) or invalid_argument (also when only a prefix is a number). Android's C++
// library has no floating-point from_chars.
std::errc parseDouble(std::string_view text, double& value);

} // namespace json
