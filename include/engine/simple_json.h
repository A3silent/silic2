#pragma once

#include <string>
#include <map>
#include <vector>
#include <variant>
#include <memory>

namespace silic2 {

// Simple JSON parser for our specific use case
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

    // Parse from string
    static SimpleJson parse(const std::string& json_str);
    
    // Serialize to string
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
    
    // Array access
    size_t size() const;
    SimpleJson& operator[](size_t index);
    const SimpleJson& operator[](size_t index) const;
    void push_back(const SimpleJson& value);

    // Object access
    SimpleJson& operator[](const std::string& key);
    const SimpleJson& operator[](const std::string& key) const;
    bool hasKey(const std::string& key) const;
    std::vector<std::string> getKeys() const;

    // Convenience methods
    double getNumber(const std::string& key, double defaultValue = 0.0) const;
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    bool getBool(const std::string& key, bool defaultValue = false) const;

private:
    Value value_;
    
    static SimpleJson parseValue(const std::string& str, size_t& pos);
    static SimpleJson parseObject(const std::string& str, size_t& pos);
    static SimpleJson parseArray(const std::string& str, size_t& pos);
    static std::string parseString(const std::string& str, size_t& pos);
    static double parseNumber(const std::string& str, size_t& pos);
    static void skipWhitespace(const std::string& str, size_t& pos);
    
    std::string valueToString() const;
    std::string escapeString(const std::string& str) const;
};

} // namespace silic2