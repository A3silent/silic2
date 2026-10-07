#pragma once

#include <string>
#include <map>
#include <vector>
#include <variant>
#include <cstddef>

namespace silic2 {

// Lightweight strict JSON value shared by the game and editor.
class SimpleJson {
public:
    enum Type {
        NULL_TYPE,
        BOOL,
        NUMBER,
        STRING,
        ARRAY,
        OBJECT
    };

    using Value = std::variant<
        std::nullptr_t,
        bool,
        double,
        std::string,
        std::vector<SimpleJson>,
        std::map<std::string, SimpleJson>
    >;

    SimpleJson() : value_(nullptr) {}
    SimpleJson(bool b) : value_(b) {}
    SimpleJson(double d) : value_(d) {}
    SimpleJson(const std::string& s) : value_(s) {}
    SimpleJson(const char* s) : value_(std::string(s)) {}

    // Parse a complete UTF-8 JSON document. Throws std::runtime_error with a
    // zero-based byte offset on malformed input; maximum container depth is 128.
    static SimpleJson parse(const std::string& json_str);
    
    static SimpleJson object();
    static SimpleJson array();

    // Serialize to string (throws for nonfinite numbers or invalid UTF-8).
    std::string toString() const;

    // Type checking
    Type getType() const;
    bool isNull() const { return getType() == NULL_TYPE; }
    bool isBool() const { return getType() == BOOL; }
    bool isNumber() const { return getType() == NUMBER; }
    bool isString() const { return getType() == STRING; }
    bool isArray() const { return getType() == ARRAY; }
    bool isObject() const { return getType() == OBJECT; }

    // Value access
    bool getBool() const;
    double getNumber() const;
    const std::string& getString() const;
    
    // Mutable indexing requires an existing array element; invalid writes throw.
    // Const lookups retain the historical null fallback.
    // Array access
    size_t size() const;
    SimpleJson& operator[](size_t index);
    const SimpleJson& operator[](size_t index) const;
    void push_back(const SimpleJson& value);

    // Object access
    SimpleJson& operator[](const std::string& key);
    const SimpleJson& operator[](const std::string& key) const;
    bool hasKey(const std::string& key) const;
    bool erase(const std::string& key);
    std::vector<std::string> getKeys() const;

    // Convenience methods
    double getNumber(const std::string& key, double defaultValue = 0.0) const;
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    bool getBool(const std::string& key, bool defaultValue = false) const;

private:
    Value value_;
    
    std::string valueToString(size_t depth = 0) const;
    std::string escapeString(const std::string& str) const;
};

} // namespace silic2
