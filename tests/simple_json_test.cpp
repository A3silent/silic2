#include "engine/simple_json.h"
#include "test_support.h"

#include <cmath>
#include <limits>
#include <locale>
#include <stdexcept>
#include <string>
#include <vector>

using silic2::SimpleJson;
using silic2::test::require;

namespace {
void rejects(const std::string& text, const std::string& offset = "") {
    try { SimpleJson::parse(text); }
    catch (const std::runtime_error& error) {
        const std::string message = error.what();
        require(message.find("JSON parse error at byte ") == 0, "missing JSON byte offset");
        require(offset.empty() || message.find("byte " + offset + ":") != std::string::npos,
                "incorrect JSON error byte offset");
        return;
    }
    throw std::runtime_error("malformed JSON accepted: " + text);
}
template<class F> void throws(F operation, const char* message) {
    try { operation(); } catch (const std::exception&) { return; }
    throw std::runtime_error(message);
}
struct CommaPunctuation : std::numpunct<char> {
    char do_decimal_point() const override { return ','; }
};
}

void runSimpleJsonTests() {
    const auto object = SimpleJson::object();
    const auto array = SimpleJson::array();
    require(object.isObject() && object.toString() == "{}", "empty object lost type");
    require(array.isArray() && array.toString() == "[]", "empty array lost type");
    require(SimpleJson::parse(" \r\n\t null ").isNull(), "valid whitespace rejected");
    const auto document = SimpleJson::parse(R"({"a":[],"b":{},"c":[true,false,null,-12.5e+2]})");
    require(document["a"].isArray() && document["b"].isObject(), "empty nested type lost");
    require(document["c"][size_t(0)].getBool() && !document["c"][size_t(1)].getBool() &&
            document["c"][size_t(2)].isNull() && document["c"][size_t(3)].getNumber() == -1250,
            "valid primitive array incorrect");
    require(SimpleJson::parse(document.toString()).toString() == document.toString(), "document roundtrip failed");

    for (const auto& invalid : std::vector<std::string>{
        "", " ", "{", "[", "{\"a\"", "{\"a\":", "{\"a\":1", "[1", "{a:1}",
        "{\"a\" 1}", "{\"a\":}", "[1 2]", "{\"a\":1 \"b\":2}", "[,1]", "[1,]", "{\"a\":1,}",
        "null true", "truex", "false0", std::string("null\0x", 6),
        "tru", "False", "undefined", "NaN", "Infinity", "+1", ".5", "-.5", "01", "-01", "1.",
        "1e", "1e+", "1e-", "--1", "0x10", "1e309", "-1e309", "\vnull", "\fnull",
        "\"unterminated", "\"\\", "\"\\x\"", "\"line\nfeed\"",
        R"("\u000")", R"("\uZZZZ")", R"("\ud800")", R"("\udc00")", R"("\ud800\u0041")",
        R"({"a":1,"a":2})", R"({"a":1,"\u0061":2})"
    }) rejects(invalid);
    rejects("[1,]", "3");
    rejects("{}x", "2");
    rejects(R"({"a":1,"a":2})", "7");
    rejects(std::string("\"a\0b\"", 5), "2");
    for (const auto& invalid : std::vector<std::string>{
        std::string("\xc0\xaf", 2), std::string("\xe0\x80\x80", 3), std::string("\xed\xa0\x80", 3),
        std::string("\xf4\x90\x80\x80", 4), std::string("\xf5\x80\x80\x80", 4),
        std::string("\x80", 1), std::string("\xe2\x82", 2), std::string("\xc2" "a", 2)
    }) {
        rejects('"' + invalid + '"');
        throws([&] { SimpleJson(invalid).toString(); }, "invalid UTF-8 serialized");
    }
    const std::string unicode = u8"中文 é € 😀 􏿿";
    require(SimpleJson::parse('"' + unicode + '"').getString() == unicode, "UTF-8 text changed");
    require(SimpleJson::parse(R"("\u4e2d\u6587 \u00e9 \u20ac \ud83d\ude00 \udbff\udfff")").getString() == unicode,
            "Unicode escape/surrogate decoding failed");
    std::string controls;
    for (int i = 0; i < 32; ++i) controls += static_cast<char>(i);
    controls += "\"\\/";
    const auto serialized = SimpleJson(controls).toString();
    for (unsigned char c : serialized) require(c >= 32, "raw control character serialized");
    require(SimpleJson::parse(serialized).getString() == controls, "control string roundtrip failed");
    require(SimpleJson::parse(R"("\b\f\n\r\t\/\\\"")").getString() == "\b\f\n\r\t/\\\"", "short escapes failed");

    for (double number : {0.0, -0.0, 1.0000000000000002, -123456789.12345679,
                          std::numeric_limits<double>::max(), std::numeric_limits<double>::min(),
                          std::numeric_limits<double>::denorm_min(), 1e-200, 1e200}) {
        const double result = SimpleJson::parse(SimpleJson(number).toString()).getNumber();
        require(result == number && std::signbit(result) == std::signbit(number), "double precision lost in roundtrip");
    }
    for (double number : {std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::quiet_NaN()})
        throws([&] { SimpleJson(number).toString(); }, "nonfinite number serialized");
    const auto previous = std::locale();
    std::locale::global(std::locale(std::locale::classic(), new CommaPunctuation));
    try {
        require(SimpleJson(1.5).toString() == "1.5" && SimpleJson::parse("1.5").getNumber() == 1.5,
                "JSON number parsing depends on application locale");
    } catch (...) { std::locale::global(previous); throw; }
    std::locale::global(previous);

    const std::string deepest = std::string(128, '[') + "0" + std::string(128, ']');
    require(SimpleJson::parse(deepest).toString() == deepest, "valid nesting boundary rejected");
    rejects('[' + deepest + ']', "128");
    auto nested = SimpleJson::array();
    nested.push_back(SimpleJson::parse(deepest));
    throws([&] { nested.toString(); }, "serializer nesting unbounded");

    auto mutableObject = SimpleJson::object();
    mutableObject["empty"] = array;
    mutableObject["keep"] = SimpleJson(5.0);
    require(mutableObject.erase("empty") && !mutableObject.erase("absent") && mutableObject.size() == 1,
            "object erase incorrect");
    auto copy = mutableObject;
    copy["keep"] = SimpleJson(8.0);
    require(mutableObject["keep"].getNumber() == 5, "copies unexpectedly share storage");
    auto mutableArray = SimpleJson::array();
    mutableArray.push_back(SimpleJson(1.0));
    mutableArray[size_t(0)] = SimpleJson(2.0);
    throws([&] { mutableArray[size_t(1)] = SimpleJson(3.0); }, "out-of-range write accepted");
    throws([&] { mutableObject[size_t(0)] = SimpleJson(3.0); }, "object accepted array index");
    throws([&] { mutableArray["key"] = SimpleJson(3.0); }, "array accepted object index");
    throws([&] { mutableObject.push_back(SimpleJson()); }, "object accepted array push");
    SimpleJson scalar(true);
    throws([&] { scalar["key"] = SimpleJson(); }, "scalar accepted object mutation");
    throws([&] { scalar.erase("key"); }, "scalar accepted object erase");
    const SimpleJson nullValue;
    require(nullValue["missing"].isNull() && nullValue[size_t(100)].isNull() &&
            nullValue.getNumber("missing", 17) == 17, "const fallback compatibility broken");
    SimpleJson implicitObject, implicitArray;
    implicitObject["key"] = SimpleJson(true);
    implicitArray.push_back(SimpleJson(true));
    require(implicitObject.isObject() && implicitArray.isArray(), "null mutation compatibility broken");
}
