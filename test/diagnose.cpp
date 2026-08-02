#include "../easyparse/json.h"
#include <iostream>
#include <cstring>

// 通过继承来访问 protected 成员
struct debugException : json::jsonException {
    using json::jsonException::jsonException;
    const char* what() const noexcept override { return msg_.data(); }
};

// 用宏 hack 来替换异常类型
#define jsonException debugException
#include "../easyparse/json.h"
#undef jsonException

// 手动 include 实际的方法实现 (因为它们是 inline 的)
// 不好搞... 换一种方式
int main() {
    // 直接测试 handle_string
    {
        json::jsonParser p("\"hello\"");
        try {
            auto v = p.parse();
            auto* s = std::get_if<std::string>(&v.value);
            std::cout << "handle_string: '" << (s ? *s : "NULL") << "'\n";
        } catch (const std::exception& e) {
            std::cout << "handle_string ERR: " << e.what() << " (type: " << typeid(e).name() << ")\n";
        }
    }
    // 测试 42
    {
        json::jsonParser p("42");
        try {
            auto v = p.parse();
            std::cout << "42: OK\n";
        } catch (const std::exception& e) {
            std::cout << "42 ERR: " << e.what() << " (type: " << typeid(e).name() << ")\n";
        }
    }
}
