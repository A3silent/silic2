#include "engine/simple_json.h"

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace silic2 {
namespace {
constexpr size_t maxDepth = 128;

[[noreturn]] void fail(size_t offset, const std::string& reason) {
    throw std::runtime_error("JSON parse error at byte " + std::to_string(offset) + ": " + reason);
}

bool digit(char c) { return c >= '0' && c <= '9'; }

// Return the length of one scalar encoded as UTF-8; reject overlong encodings,
// surrogate scalars and values outside Unicode. Offsets always refer to bytes.
size_t utf8Length(const std::string& text, size_t pos) {
    const auto first = static_cast<unsigned char>(text[pos]);
    size_t length = 0;
    uint32_t scalar = 0;
    if (first < 0x80) return 1;
    if (first >= 0xc2 && first <= 0xdf) { length = 2; scalar = first & 0x1f; }
    else if (first >= 0xe0 && first <= 0xef) { length = 3; scalar = first & 0x0f; }
    else if (first >= 0xf0 && first <= 0xf4) { length = 4; scalar = first & 0x07; }
    else fail(pos, "invalid UTF-8 leading byte");
    if (text.size() - pos < length) fail(pos, "truncated UTF-8 sequence");
    for (size_t i = 1; i < length; ++i) {
        const auto next = static_cast<unsigned char>(text[pos + i]);
        if ((next & 0xc0) != 0x80) fail(pos + i, "invalid UTF-8 continuation byte");
        scalar = (scalar << 6) | (next & 0x3f);
    }
    if ((length == 3 && scalar < 0x800) || (length == 4 && scalar < 0x10000) ||
        (scalar >= 0xd800 && scalar <= 0xdfff) || scalar > 0x10ffff)
        fail(pos, "invalid Unicode scalar in UTF-8");
    return length;
}

void appendUtf8(std::string& out, uint32_t scalar) {
    if (scalar < 0x80) out += static_cast<char>(scalar);
    else if (scalar < 0x800) {
        out += static_cast<char>(0xc0 | (scalar >> 6));
        out += static_cast<char>(0x80 | (scalar & 0x3f));
    } else if (scalar < 0x10000) {
        out += static_cast<char>(0xe0 | (scalar >> 12));
        out += static_cast<char>(0x80 | ((scalar >> 6) & 0x3f));
        out += static_cast<char>(0x80 | (scalar & 0x3f));
    } else {
        out += static_cast<char>(0xf0 | (scalar >> 18));
        out += static_cast<char>(0x80 | ((scalar >> 12) & 0x3f));
        out += static_cast<char>(0x80 | ((scalar >> 6) & 0x3f));
        out += static_cast<char>(0x80 | (scalar & 0x3f));
    }
}

class Parser {
public:
    explicit Parser(const std::string& input) : text(input) {}
    SimpleJson document() {
        auto result = value(0);
        whitespace();
        if (pos != text.size()) fail(pos, "trailing data after JSON value");
        return result;
    }
private:
    const std::string& text;
    size_t pos = 0;

    void whitespace() {
        while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t' ||
               text[pos] == '\r' || text[pos] == '\n')) ++pos;
    }
    bool take(char c) {
        if (pos < text.size() && text[pos] == c) { ++pos; return true; }
        return false;
    }
    void expect(char c, const char* reason) { if (!take(c)) fail(pos, reason); }
    void literal(const char* word) {
        for (const char* p = word; *p; ++p) expect(*p, "invalid literal");
    }
    uint32_t hex4() {
        uint32_t result = 0;
        for (int i = 0; i < 4; ++i) {
            if (pos == text.size()) fail(pos, "incomplete Unicode escape");
            const char c = text[pos];
            uint32_t n;
            if (digit(c)) n = c - '0';
            else if (c >= 'a' && c <= 'f') n = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') n = c - 'A' + 10;
            else fail(pos, "expected hexadecimal digit");
            ++pos;
            result = (result << 4) | n;
        }
        return result;
    }
    std::string string() {
        expect('"', "expected string");
        std::string result;
        while (pos < text.size()) {
            if (take('"')) return result;
            const auto c = static_cast<unsigned char>(text[pos]);
            if (c < 0x20) fail(pos, "unescaped control character in string");
            if (take('\\')) {
                if (pos == text.size()) fail(pos, "incomplete escape");
                const size_t escapeOffset = pos;
                switch (text[pos++]) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'u': {
                        uint32_t scalar = hex4();
                        if (scalar >= 0xd800 && scalar <= 0xdbff) {
                            expect('\\', "high surrogate requires a low surrogate escape");
                            expect('u', "high surrogate requires a low surrogate escape");
                            const size_t lowOffset = pos;
                            const uint32_t low = hex4();
                            if (low < 0xdc00 || low > 0xdfff) fail(lowOffset, "expected low surrogate");
                            scalar = 0x10000 + ((scalar - 0xd800) << 10) + low - 0xdc00;
                        } else if (scalar >= 0xdc00 && scalar <= 0xdfff)
                            fail(escapeOffset, "unpaired low surrogate");
                        appendUtf8(result, scalar);
                        break;
                    }
                    default: fail(escapeOffset, "invalid string escape");
                }
            } else {
                const size_t length = utf8Length(text, pos);
                result.append(text, pos, length);
                pos += length;
            }
        }
        fail(pos, "unterminated string");
    }
    double number() {
        const size_t start = pos;
        take('-');
        if (take('0')) {
            if (pos < text.size() && digit(text[pos])) fail(pos, "leading zero in number");
        } else {
            if (pos == text.size() || text[pos] < '1' || text[pos] > '9') fail(pos, "expected number digit");
            while (pos < text.size() && digit(text[pos])) ++pos;
        }
        if (take('.')) {
            if (pos == text.size() || !digit(text[pos])) fail(pos, "expected fraction digit");
            while (pos < text.size() && digit(text[pos])) ++pos;
        }
        if (take('e') || take('E')) {
            if (!take('+')) take('-');
            if (pos == text.size() || !digit(text[pos])) fail(pos, "expected exponent digit");
            while (pos < text.size() && digit(text[pos])) ++pos;
        }
        std::istringstream input(text.substr(start, pos - start));
        input.imbue(std::locale::classic());
        double result = 0;
        input >> result;
        // libc++ reports ERANGE via failbit even for representable subnormals.
        // Keep those values for exact roundtrips; overflow (saturated max) and
        // complete underflow remain explicit range errors.
        const bool subnormal = result != 0.0 && std::abs(result) < std::numeric_limits<double>::min();
        if ((input.fail() && !subnormal) || !std::isfinite(result))
            fail(start, "number outside finite double range");
        return result;
    }
    SimpleJson value(size_t depth) {
        whitespace();
        if (pos == text.size()) fail(pos, "expected value");
        const char c = text[pos];
        if (c == '{' || c == '[') {
            if (depth >= maxDepth) fail(pos, "maximum nesting depth (128) exceeded");
            ++pos;
            whitespace();
            auto result = c == '{' ? SimpleJson::object() : SimpleJson::array();
            const char closing = c == '{' ? '}' : ']';
            if (take(closing)) return result;
            while (true) {
                if (c == '{') {
                    whitespace();
                    const size_t keyOffset = pos;
                    const std::string key = string();
                    if (result.hasKey(key)) fail(keyOffset, "duplicate object key");
                    whitespace();
                    expect(':', "expected ':' after object key");
                    result[key] = value(depth + 1);
                } else result.push_back(value(depth + 1));
                whitespace();
                if (take(closing)) return result;
                expect(',', "expected ',' or container end");
            }
        }
        if (c == '"') return SimpleJson(string());
        if (c == 't') { literal("true"); return SimpleJson(true); }
        if (c == 'f') { literal("false"); return SimpleJson(false); }
        if (c == 'n') { literal("null"); return SimpleJson(); }
        if (c == '-' || digit(c)) return SimpleJson(number());
        fail(pos, "expected JSON value");
    }
};
} // namespace

SimpleJson SimpleJson::parse(const std::string& input) { return Parser(input).document(); }
SimpleJson SimpleJson::object() { SimpleJson result; result.value_ = std::map<std::string, SimpleJson>(); return result; }
SimpleJson SimpleJson::array() { SimpleJson result; result.value_ = std::vector<SimpleJson>(); return result; }
std::string SimpleJson::toString() const { return valueToString(); }

SimpleJson::Type SimpleJson::getType() const { return static_cast<Type>(value_.index()); }
bool SimpleJson::getBool() const {
    const auto* value = std::get_if<bool>(&value_);
    return value ? *value : false;
}
double SimpleJson::getNumber() const {
    const auto* value = std::get_if<double>(&value_);
    return value ? *value : 0.0;
}
const std::string& SimpleJson::getString() const {
    if (const auto* value = std::get_if<std::string>(&value_)) return *value;
    static const std::string empty;
    return empty;
}
size_t SimpleJson::size() const {
    if (const auto* value = std::get_if<std::vector<SimpleJson>>(&value_)) return value->size();
    if (const auto* value = std::get_if<std::map<std::string, SimpleJson>>(&value_)) return value->size();
    return 0;
}
SimpleJson& SimpleJson::operator[](size_t index) {
    if (auto* values = std::get_if<std::vector<SimpleJson>>(&value_)) return values->at(index);
    throw std::logic_error("JSON array indexing requires an array");
}
const SimpleJson& SimpleJson::operator[](size_t index) const {
    if (const auto* values = std::get_if<std::vector<SimpleJson>>(&value_))
        if (index < values->size()) return (*values)[index];
    static const SimpleJson nullValue;
    return nullValue;
}
void SimpleJson::push_back(const SimpleJson& value) {
    if (isNull()) value_ = std::vector<SimpleJson>();
    if (auto* values = std::get_if<std::vector<SimpleJson>>(&value_)) { values->push_back(value); return; }
    throw std::logic_error("JSON push_back requires an array or null");
}
SimpleJson& SimpleJson::operator[](const std::string& key) {
    if (isNull()) value_ = std::map<std::string, SimpleJson>();
    if (auto* values = std::get_if<std::map<std::string, SimpleJson>>(&value_)) return (*values)[key];
    throw std::logic_error("JSON object indexing requires an object or null");
}
const SimpleJson& SimpleJson::operator[](const std::string& key) const {
    if (const auto* values = std::get_if<std::map<std::string, SimpleJson>>(&value_)) {
        const auto it = values->find(key);
        if (it != values->end()) return it->second;
    }
    static const SimpleJson nullValue;
    return nullValue;
}
bool SimpleJson::hasKey(const std::string& key) const {
    const auto* values = std::get_if<std::map<std::string, SimpleJson>>(&value_);
    return values && values->find(key) != values->end();
}
bool SimpleJson::erase(const std::string& key) {
    if (auto* values = std::get_if<std::map<std::string, SimpleJson>>(&value_)) return values->erase(key) != 0;
    if (isNull()) return false;
    throw std::logic_error("JSON erase requires an object or null");
}
std::vector<std::string> SimpleJson::getKeys() const {
    std::vector<std::string> keys;
    if (const auto* values = std::get_if<std::map<std::string, SimpleJson>>(&value_))
        for (const auto& entry : *values) keys.push_back(entry.first);
    return keys;
}
double SimpleJson::getNumber(const std::string& key, double fallback) const {
    const auto& value = (*this)[key];
    return value.isNumber() ? value.getNumber() : fallback;
}
std::string SimpleJson::getString(const std::string& key, const std::string& fallback) const {
    const auto& value = (*this)[key];
    return value.isString() ? value.getString() : fallback;
}
bool SimpleJson::getBool(const std::string& key, bool fallback) const {
    const auto& value = (*this)[key];
    return value.isBool() ? value.getBool() : fallback;
}

std::string SimpleJson::valueToString(size_t depth) const {
    if ((isArray() || isObject()) && depth >= maxDepth)
        throw std::runtime_error("JSON serialization exceeds maximum nesting depth (128)");
    switch (getType()) {
        case NULL_TYPE: return "null";
        case BOOL: return getBool() ? "true" : "false";
        case NUMBER: {
            if (!std::isfinite(getNumber())) throw std::runtime_error("JSON cannot serialize a nonfinite number");
            std::ostringstream output;
            output.imbue(std::locale::classic());
            output << std::setprecision(std::numeric_limits<double>::max_digits10) << getNumber();
            return output.str();
        }
        case STRING: return "\"" + escapeString(getString()) + "\"";
        case ARRAY: {
            std::string result = "[";
            bool first = true;
            for (const auto& entry : std::get<std::vector<SimpleJson>>(value_)) {
                if (!first) result += ',';
                first = false;
                result += entry.valueToString(depth + 1);
            }
            return result + ']';
        }
        case OBJECT: {
            std::string result = "{";
            bool first = true;
            for (const auto& entry : std::get<std::map<std::string, SimpleJson>>(value_)) {
                if (!first) result += ',';
                first = false;
                result += '"' + escapeString(entry.first) + "\":" + entry.second.valueToString(depth + 1);
            }
            return result + '}';
        }
    }
    throw std::logic_error("Invalid JSON value type");
}
std::string SimpleJson::escapeString(const std::string& text) const {
    static constexpr char hex[] = "0123456789abcdef";
    std::string result;
    for (size_t pos = 0; pos < text.size();) {
        const auto c = static_cast<unsigned char>(text[pos]);
        if (c < 0x20) {
            result += "\\u00";
            result += hex[c >> 4];
            result += hex[c & 0xf];
            ++pos;
        } else if (c == '"' || c == '\\') {
            result += '\\';
            result += static_cast<char>(c);
            ++pos;
        } else {
            const size_t length = utf8Length(text, pos);
            result.append(text, pos, length);
            pos += length;
        }
    }
    return result;
}
} // namespace silic2
