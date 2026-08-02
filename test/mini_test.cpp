#include "../easyparse/json.h"
#include <iostream>

int main() {
    // 逐个测试，捕获 jsonException 和 std::exception 分别处理
    const char* tests[] = {
        "\"hello\"", "42", "-128", "true", "false", "null", "{}", "[]", "3.14", "[1]",
    };
    for (auto t : tests) {
        try {
            auto v = json::jsonParser(t).parse();
            std::cout << "OK: '" << t << "'\n";
        } catch (const json::jsonException& e) {
            // msg_ is protected, can't access. Try dynamic_cast
            std::cout << "JSON_ERR: '" << t << "' — what()='" << e.what() << "'\n";
        } catch (const std::exception& e) {
            std::cout << "STD_ERR: '" << t << "' — what()='" << e.what() << "'\n";
        }
    }
}
