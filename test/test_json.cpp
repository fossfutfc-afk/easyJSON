/**
 * easyparse json.h — 测试套件
 *
 * 构建:  cd json/test && g++ -std=c++17 -Wall -o test_json test_json.cpp
 * 用法:  ./test_json               — 运行所有测试
 */

#include "../easyparse/json.h"
#include <iostream>
#include <sstream>
#include <cmath>

// ============================================================
// 轻量测试框架
// ============================================================
static int g_passed = 0;
static int g_failed = 0;

static void pass() { std::cout << "PASSED\n"; ++g_passed; }
static void fail(const std::string& msg) { std::cout << "FAILED — " << msg << "\n"; ++g_failed; }

#define TEST(name) std::cout << "  [" << #name << "] ";

static std::string type_name(const json::jsonValue& v) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>)  return "null";
        else if constexpr (std::is_same_v<T, int>)         return "int";
        else if constexpr (std::is_same_v<T, double>)      return "double";
        else if constexpr (std::is_same_v<T, std::string>)  return "string";
        else if constexpr (std::is_same_v<T, bool>)         return "bool";
        else if constexpr (std::is_same_v<T, json::jsonValue::arr>) return "array";
        else if constexpr (std::is_same_v<T, json::jsonValue::obj>) return "object";
        else return "unknown";
    }, v.value);
}

// ============================================================
// 基础类型
// ============================================================
void test_null() {
    TEST(null);
    try {
        auto v = json::jsonParser("null").parse();
        if (std::holds_alternative<std::monostate>(v.value)) pass();
        else fail("expected null, got " + type_name(v));
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_true() {
    TEST(true);
    try {
        auto v = json::jsonParser("true").parse();
        auto* b = std::get_if<bool>(&v.value);
        if (b && *b) pass(); else fail("expected true");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_false() {
    TEST(false);
    try {
        auto v = json::jsonParser("false").parse();
        auto* b = std::get_if<bool>(&v.value);
        if (b && !*b) pass(); else fail("expected false");
    } catch (const std::exception& e) { fail(e.what()); }
}

// ============================================================
// 字符串
// ============================================================
void test_string() {
    TEST(string);
    try {
        auto v = json::jsonParser("\"hello\"").parse();
        auto* s = std::get_if<std::string>(&v.value);
        if (s && *s == "hello") pass();
        else fail(s ? "got '" + *s + "'" : "not a string");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_empty_string() {
    TEST(empty_string);
    try {
        auto v = json::jsonParser("\"\"").parse();
        auto* s = std::get_if<std::string>(&v.value);
        if (s && s->empty()) pass(); else fail("expected empty string");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_string_escape_quote() {
    TEST(escape_quote);
    try {
        auto v = json::jsonParser("\"hello\\\"world\"").parse();
        auto* s = std::get_if<std::string>(&v.value);
        if (s && *s == "hello\"world") pass();
        else fail(s ? "got '" + *s + "'" : "not a string");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_string_escape_special() {
    TEST(escape_special);
    try {
        auto v = json::jsonParser("\"a\\nb\\tc\"").parse();
        auto* s = std::get_if<std::string>(&v.value);
        if (s && *s == "a\nb\tc") pass();
        else fail("expected 'a\\nb\\tc'");
    } catch (const std::exception& e) { fail(e.what()); }
}

// ============================================================
// 数字
// ============================================================
void test_int() {
    TEST(int);
    try {
        auto v = json::jsonParser("42").parse();
        auto* i = std::get_if<int>(&v.value);
        if (i && *i == 42) pass(); else fail("expected 42");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_negative_int() {
    TEST(negative_int);
    try {
        auto v = json::jsonParser("-128").parse();
        auto* i = std::get_if<int>(&v.value);
        if (i && *i == -128) pass(); else fail("expected -128");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_double() {
    TEST(double);
    try {
        auto v = json::jsonParser("3.14").parse();
        auto* d = std::get_if<double>(&v.value);
        if (d && std::abs(*d - 3.14) < 0.001) pass();
        else fail("expected 3.14");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_scientific_notation() {
    TEST(scientific);
    try {
        auto v = json::jsonParser("1.5e3").parse();
        auto* d = std::get_if<double>(&v.value);
        if (d && std::abs(*d - 1500.0) < 0.001) pass();
        else fail("expected 1500");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_sci_negative_exp() {
    TEST(sci_negative);
    try {
        auto v = json::jsonParser("3.0E-2").parse();
        auto* d = std::get_if<double>(&v.value);
        if (d && std::abs(*d - 0.03) < 0.0001) pass();
        else fail("expected 0.03");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_zero_with_decimal() {
    TEST(zero_decimal);
    try {
        auto v = json::jsonParser("0.5").parse();
        auto* d = std::get_if<double>(&v.value);
        if (d && std::abs(*d - 0.5) < 0.001) pass();
        else fail("expected 0.5");
    } catch (const std::exception& e) { fail(e.what()); }
}

// ============================================================
// 数字边界 (应被拒绝)
// ============================================================
void test_leading_zero_rejected() {
    TEST(leading_zero);
    try {
        json::jsonParser("0123").parse();
        fail("0123 should be rejected");
    } catch (const json::jsonException&) { pass(); }
    catch (const std::exception& e) { fail(e.what()); }
}

void test_overflow_rejected() {
    TEST(overflow);
    try {
        json::jsonParser("1e999").parse();
        fail("1e999 should be rejected");
    } catch (const json::jsonException&) { pass(); }
    catch (const std::exception& e) { fail(e.what()); }
}

// ============================================================
// 数组
// ============================================================
void test_empty_array() {
    TEST(empty_array);
    try {
        auto v = json::jsonParser("[]").parse();
        auto* arr = std::get_if<json::jsonValue::arr>(&v.value);
        if (arr && arr->empty()) pass(); else fail("expected empty array");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_single_element_array() {
    TEST(single_element);
    try {
        auto v = json::jsonParser("[1]").parse();
        auto* arr = std::get_if<json::jsonValue::arr>(&v.value);
        if (arr && arr->size() == 1) pass();
        else fail("expected [1]");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_multi_element_array() {
    TEST(multi_element);
    try {
        auto v = json::jsonParser("[1, 2, 3]").parse();
        auto* arr = std::get_if<json::jsonValue::arr>(&v.value);
        if (arr && arr->size() == 3) pass();
        else fail("expected [1,2,3]");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_nested_array() {
    TEST(nested_array);
    try {
        auto v = json::jsonParser("[1, [2, 3]]").parse();
        auto* arr = std::get_if<json::jsonValue::arr>(&v.value);
        if (!arr) { fail("not an array"); return; }
        if (arr->size() != 2) { fail("expected size 2"); return; }
        auto* inner = std::get_if<json::jsonValue::arr>(&(*arr)[1].value);
        if (inner && inner->size() == 2) pass();
        else fail("expected nested [2,3]");
    } catch (const std::exception& e) { fail(e.what()); }
}

// ============================================================
// 对象
// ============================================================
void test_empty_object() {
    TEST(empty_object);
    try {
        auto v = json::jsonParser("{}").parse();
        auto* obj = std::get_if<json::jsonValue::obj>(&v.value);
        if (obj && obj->empty()) pass(); else fail("expected {}");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_simple_object() {
    TEST(simple_object);
    try {
        auto v = json::jsonParser("{\"a\": 1}").parse();
        auto* obj = std::get_if<json::jsonValue::obj>(&v.value);
        if (!obj || obj->size() != 1) { fail("expected size 1"); return; }
        auto it = obj->find("a");
        auto* i = std::get_if<int>(&it->second.value);
        if (i && *i == 1) pass(); else fail("value not 1");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_multi_key_object() {
    TEST(multi_key);
    try {
        auto v = json::jsonParser("{\"name\": \"Alice\", \"age\": 30}").parse();
        auto* obj = std::get_if<json::jsonValue::obj>(&v.value);
        if (!obj || obj->size() != 2) { fail("expected size 2"); return; }
        if (obj->count("name") && obj->count("age")) pass();
        else fail("missing keys");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_nested_object() {
    TEST(nested_object);
    try {
        auto v = json::jsonParser(
            "{\"name\":\"Alice\",\"age\":30,\"scores\":[95,87,92]}"
        ).parse();
        auto* obj = std::get_if<json::jsonValue::obj>(&v.value);
        if (!obj) { fail("not an object"); return; }
        bool ok = obj->count("name") && obj->count("age") && obj->count("scores");
        if (ok) pass(); else fail("missing keys");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_duplicate_key_rejected() {
    TEST(duplicate_key);
    try {
        json::jsonParser("{\"a\":1,\"a\":2}").parse();
        fail("duplicate key should be rejected");
    } catch (const json::jsonException&) { pass(); }
    catch (const std::exception& e) { fail(e.what()); }
}

// ============================================================
// 空格
// ============================================================
void test_leading_whitespace() {
    TEST(leading_ws);
    try {
        auto v = json::jsonParser("  42").parse();
        auto* i = std::get_if<int>(&v.value);
        if (i && *i == 42) pass(); else fail("expected 42");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_json_with_spaces() {
    TEST(spaces);
    try {
        auto v = json::jsonParser("{ \"a\" : 1 }").parse();
        auto* obj = std::get_if<json::jsonValue::obj>(&v.value);
        if (obj && obj->size() == 1) pass();
        else fail("expected object");
    } catch (const std::exception& e) { fail(e.what()); }
}

// ============================================================
// 异常
// ============================================================
void test_exception_what() {
    TEST(exception_what);
    try {
        json::jsonParser("INVALID").parse();
        fail("expected exception");
    } catch (const json::jsonException& e) {
        if (e.what()[0] != '\0') pass();
        else fail("what() empty");
    } catch (const std::exception& e) { fail(e.what()); }
}

void test_empty_input() {
    TEST(empty_input);
    try {
        json::jsonParser("").parse();
        fail("expected exception");
    } catch (const json::jsonException&) { pass(); }
    catch (const std::exception& e) { fail(e.what()); }
}

// ============================================================
// main
// ============================================================
int main() {
    std::cout << "================================================\n";
    std::cout << "  easyparse json.h  测试套件\n";
    std::cout << "================================================\n\n";

    std::cout << "——— 基础类型 ———\n";
    test_null();
    test_true();
    test_false();

    std::cout << "\n——— 字符串 ———\n";
    test_string();
    test_empty_string();
    test_string_escape_quote();
    test_string_escape_special();

    std::cout << "\n——— 数字 ———\n";
    test_int();
    test_negative_int();
    test_double();
    test_scientific_notation();
    test_sci_negative_exp();
    test_zero_with_decimal();

    std::cout << "\n——— 数字边界 ———\n";
    test_leading_zero_rejected();
    test_overflow_rejected();

    std::cout << "\n——— 数组 ———\n";
    test_empty_array();
    test_single_element_array();
    test_multi_element_array();
    test_nested_array();

    std::cout << "\n——— 对象 ———\n";
    test_empty_object();
    test_simple_object();
    test_multi_key_object();
    test_nested_object();
    test_duplicate_key_rejected();

    std::cout << "\n——— 空格 ———\n";
    test_leading_whitespace();
    test_json_with_spaces();

    std::cout << "\n——— 异常 ———\n";
    test_exception_what();
    test_empty_input();

    std::cout << "\n================================================\n";
    std::cout << "  结果: " << g_passed << " 通过, "
              << g_failed << " 失败 (共 " << (g_passed + g_failed) << ")\n";
    std::cout << "================================================\n";

    return g_failed > 0 ? 1 : 0;
}
