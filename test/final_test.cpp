// 临时将 msg_ 改为 public 来看到真正的错误信息
#define protected public
#include "../easyparse/json.h"
#undef protected
#include <iostream>

int main() {
    const char* inputs[] = {
        "\"x\"", "42", "-128", "true", "false", "null", "{}", "[]",
        "3.14", "[1]", "{\"k\":1}", " 42", "\"a\\\"b\"",
    };
    for (auto input : inputs) {
        try {
            auto v = json::jsonParser(input).parse();
            std::cout << "OK: " << input << "\n";
        } catch (const json::jsonException& e) {
            std::cout << "FAIL: " << input << " → msg_=\"" << e.what() << "\"\n";
        } catch (const std::exception& e) {
            std::cout << "STD: " << input << " → " << e.what() << "\n";
        }
    }
}
