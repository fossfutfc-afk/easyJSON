#include "../easyparse/json.h"
#include <iostream>

int main() {
    // 测试1: 最简单的字符串
    try {
        auto v = json::jsonParser("\"hi\"").parse();
        auto* s = std::get_if<std::string>(&v.value);
        std::cout << "Test1 PASS: '" << (s ? *s : "NULL") << "'\n";
    } catch (const std::exception& e) {
        std::cout << "Test1 FAIL: " << e.what() << "\n";
    }

    // 测试2: 直接测试 handle_string
    try {
        json::jsonParser p("\"hi\"");
        auto str = p.parse();  // should trigger handle_string
        std::cout << "Test2 PASS\n";
    } catch (const std::exception& e) {
        std::cout << "Test2 FAIL: " << e.what() << "\n";
    }

    // 测试3: 检查 jsonException 的 what()
    try {
        throw json::jsonException("custom error msg");
    } catch (const std::exception& e) {
        std::cout << "Test3 what() returns: '" << e.what() << "'\n";
    }

    // 测试4: 检查 jsonException 是否被正确捕获
    try {
        json::jsonParser("!!!").parse();
    } catch (const json::jsonException& e) {
        std::cout << "Test4 caught jsonException: '" << e.what() << "'\n";
    } catch (const std::exception& e) {
        std::cout << "Test4 caught std::exception: '" << e.what() << "'\n";
    }
}
