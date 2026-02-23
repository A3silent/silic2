#include "engine/simple_json.h"
#include <sstream>
#include <cctype>
#include <iostream>

namespace silic2 {

SimpleJson SimpleJson::parse(const std::string& json_str) {
    size_t pos = 0;
    skipWhitespace(json_str, pos);
    if (pos >= json_str.length()) {
        return SimpleJson();
    }
    return parseValue(json_str, pos);
}

std::string SimpleJson::toString() const {
    return valueToString();
}

SimpleJson::Type SimpleJson::getType() const {
    if (std::holds_alternative<std::nullptr_t>(value_)) return NULL_TYPE;
    if (std::holds_alternative<bool>(value_)) return BOOL;
    if (std::holds_alternative<double>(value_)) return NUMBER;
    if (std::holds_alternative<std::string>(value_)) return STRING;
    if (std::holds_alternative<std::vector<SimpleJson>>(value_)) return ARRAY;
    if (std::holds_alternative<std::map<std::string, SimpleJson>>(value_)) return OBJECT;
    return NULL_TYPE;
}

bool SimpleJson::getBool() const {
    if (auto* b = std::get_if<bool>(&value_)) {
        return *b;
    }
    return false;
}

double SimpleJson::getNumber() const {
    if (auto* d = std::get_if<double>(&value_)) {
        return *d;
    }
    return 0.0;
}

const std::string& SimpleJson::getString() const {
    if (auto* s = std::get_if<std::string>(&value_)) {
        return *s;
    }
    static std::string empty;
    return empty;
}

size_t SimpleJson::size() const {
    if (auto* arr = std::get_if<std::vector<SimpleJson>>(&value_)) {
        return arr->size();
    }
    if (auto* obj = std::get_if<std::map<std::string, SimpleJson>>(&value_)) {
        return obj->size();
    }
    return 0;
}

SimpleJson& SimpleJson::operator[](size_t index) {
    if (auto* arr = std::get_if<std::vector<SimpleJson>>(&value_)) {
        if (index < arr->size()) {
            return (*arr)[index];
        }
    }
    if (getType() == NULL_TYPE) {
        value_ = std::vector<SimpleJson>();
        auto* arr = std::get_if<std::vector<SimpleJson>>(&value_);
        arr->resize(index + 1);
        return (*arr)[index];
    }
    static SimpleJson null_json;
    return null_json;
}

const SimpleJson& SimpleJson::operator[](size_t index) const {
    if (auto* arr = std::get_if<std::vector<SimpleJson>>(&value_)) {
        if (index < arr->size()) {
            return (*arr)[index];
        }
    }
    static SimpleJson null_json;
    return null_json;
}

void SimpleJson::push_back(const SimpleJson& value) {
    if (getType() == NULL_TYPE) {
        value_ = std::vector<SimpleJson>();
    }
    if (auto* arr = std::get_if<std::vector<SimpleJson>>(&value_)) {
        arr->push_back(value);
    }
}

SimpleJson& SimpleJson::operator[](const std::string& key) {
    if (getType() == NULL_TYPE) {
        value_ = std::map<std::string, SimpleJson>();
    }
    if (auto* obj = std::get_if<std::map<std::string, SimpleJson>>(&value_)) {
        return (*obj)[key];
    }
    static SimpleJson null_json;
    return null_json;
}

const SimpleJson& SimpleJson::operator[](const std::string& key) const {
    if (auto* obj = std::get_if<std::map<std::string, SimpleJson>>(&value_)) {
        auto it = obj->find(key);
        if (it != obj->end()) {
            return it->second;
        }
    }
    static SimpleJson null_json;
    return null_json;
}

bool SimpleJson::hasKey(const std::string& key) const {
    if (auto* obj = std::get_if<std::map<std::string, SimpleJson>>(&value_)) {
        return obj->find(key) != obj->end();
    }
    return false;
}

std::vector<std::string> SimpleJson::getKeys() const {
    std::vector<std::string> keys;
    if (auto* obj = std::get_if<std::map<std::string, SimpleJson>>(&value_)) {
        for (const auto& pair : *obj) {
            keys.push_back(pair.first);
        }
    }
    return keys;
}

double SimpleJson::getNumber(const std::string& key, double defaultValue) const {
    const SimpleJson& val = (*this)[key];
    return val.isNumber() ? val.getNumber() : defaultValue;
}

std::string SimpleJson::getString(const std::string& key, const std::string& defaultValue) const {
    const SimpleJson& val = (*this)[key];
    return val.isString() ? val.getString() : defaultValue;
}

bool SimpleJson::getBool(const std::string& key, bool defaultValue) const {
    const SimpleJson& val = (*this)[key];
    return val.isBool() ? val.getBool() : defaultValue;
}

SimpleJson SimpleJson::parseValue(const std::string& str, size_t& pos) {
    skipWhitespace(str, pos);
    if (pos >= str.length()) return SimpleJson();
    
    char c = str[pos];
    
    if (c == '{') {
        return parseObject(str, pos);
    } else if (c == '[') {
        return parseArray(str, pos);
    } else if (c == '"') {
        return SimpleJson(parseString(str, pos));
    } else if (c == 't' || c == 'f') {
        // Boolean
        if (str.substr(pos, 4) == "true") {
            pos += 4;
            return SimpleJson(true);
        } else if (str.substr(pos, 5) == "false") {
            pos += 5;
            return SimpleJson(false);
        }
    } else if (c == 'n') {
        // Null
        if (str.substr(pos, 4) == "null") {
            pos += 4;
            return SimpleJson();
        }
    } else if (c == '-' || std::isdigit(c)) {
        return SimpleJson(parseNumber(str, pos));
    }
    
    return SimpleJson();
}

SimpleJson SimpleJson::parseObject(const std::string& str, size_t& pos) {
    SimpleJson obj;
    obj.value_ = std::map<std::string, SimpleJson>();
    
    pos++; // Skip '{'
    skipWhitespace(str, pos);
    
    if (pos < str.length() && str[pos] == '}') {
        pos++; // Empty object
        return obj;
    }
    
    while (pos < str.length()) {
        skipWhitespace(str, pos);
        
        // Parse key
        if (str[pos] != '"') break;
        std::string key = parseString(str, pos);
        
        skipWhitespace(str, pos);
        if (pos >= str.length() || str[pos] != ':') break;
        pos++; // Skip ':'
        
        // Parse value
        SimpleJson value = parseValue(str, pos);
        obj[key] = value;
        
        skipWhitespace(str, pos);
        if (pos >= str.length()) break;
        
        if (str[pos] == '}') {
            pos++;
            break;
        } else if (str[pos] == ',') {
            pos++;
        } else {
            break;
        }
    }
    
    return obj;
}

SimpleJson SimpleJson::parseArray(const std::string& str, size_t& pos) {
    SimpleJson arr;
    arr.value_ = std::vector<SimpleJson>();
    
    pos++; // Skip '['
    skipWhitespace(str, pos);
    
    if (pos < str.length() && str[pos] == ']') {
        pos++; // Empty array
        return arr;
    }
    
    while (pos < str.length()) {
        SimpleJson value = parseValue(str, pos);
        arr.push_back(value);
        
        skipWhitespace(str, pos);
        if (pos >= str.length()) break;
        
        if (str[pos] == ']') {
            pos++;
            break;
        } else if (str[pos] == ',') {
            pos++;
        } else {
            break;
        }
    }
    
    return arr;
}

std::string SimpleJson::parseString(const std::string& str, size_t& pos) {
    if (pos >= str.length() || str[pos] != '"') return "";
    
    pos++; // Skip opening quote
    std::string result;
    
    while (pos < str.length() && str[pos] != '"') {
        if (str[pos] == '\\' && pos + 1 < str.length()) {
            pos++;
            char escaped = str[pos];
            switch (escaped) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default: result += escaped; break;
            }
        } else {
            result += str[pos];
        }
        pos++;
    }
    
    if (pos < str.length()) pos++; // Skip closing quote
    return result;
}

double SimpleJson::parseNumber(const std::string& str, size_t& pos) {
    size_t start = pos;
    
    if (pos < str.length() && str[pos] == '-') pos++;
    
    while (pos < str.length() && std::isdigit(str[pos])) pos++;
    
    if (pos < str.length() && str[pos] == '.') {
        pos++;
        while (pos < str.length() && std::isdigit(str[pos])) pos++;
    }
    
    if (pos < str.length() && (str[pos] == 'e' || str[pos] == 'E')) {
        pos++;
        if (pos < str.length() && (str[pos] == '+' || str[pos] == '-')) pos++;
        while (pos < str.length() && std::isdigit(str[pos])) pos++;
    }
    
    return std::stod(str.substr(start, pos - start));
}

void SimpleJson::skipWhitespace(const std::string& str, size_t& pos) {
    while (pos < str.length() && std::isspace(str[pos])) {
        pos++;
    }
}

std::string SimpleJson::valueToString() const {
    switch (getType()) {
        case NULL_TYPE:
            return "null";
        case BOOL:
            return getBool() ? "true" : "false";
        case NUMBER: {
            std::ostringstream oss;
            oss << getNumber();
            return oss.str();
        }
        case STRING:
            return "\"" + escapeString(getString()) + "\"";
        case ARRAY: {
            std::string result = "[";
            auto* arr = std::get_if<std::vector<SimpleJson>>(&value_);
            for (size_t i = 0; i < arr->size(); ++i) {
                if (i > 0) result += ",";
                result += (*arr)[i].valueToString();
            }
            result += "]";
            return result;
        }
        case OBJECT: {
            std::string result = "{";
            auto* obj = std::get_if<std::map<std::string, SimpleJson>>(&value_);
            bool first = true;
            for (const auto& pair : *obj) {
                if (!first) result += ",";
                result += "\"" + escapeString(pair.first) + "\":" + pair.second.valueToString();
                first = false;
            }
            result += "}";
            return result;
        }
    }
    return "";
}

std::string SimpleJson::escapeString(const std::string& str) const {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

} // namespace silic2