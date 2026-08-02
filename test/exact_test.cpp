#include "../easyparse/json.h"
#include <iostream>

int main() {
    try {
        auto v = json::jsonParser("\"x\"").parse();
        std::cout << "string 'x': OK\n";
    } catch (const std::exception& e) {
        std::cout << "string 'x': ERR - " << e.what() << "\n";
    }

    try {
        auto v = json::jsonParser("42").parse();
        auto* d = std::get_if<double>(&v.value);
        auto* i = std::get_if<int>(&v.value);
        std::cout << "42: double*=" << (void*)d << " int*=" << (void*)i << "\n";
        if (d) std::cout << "  -> double: " << *d << "\n";
        if (i) std::cout << "  -> int: " << *i << "\n";
    } catch (const std::exception& e) {
        std::cout << "42: ERR - " << e.what() << "\n";
    }
    
    // 关键验证: try_consume('\0') 在文件末尾的行为
    // 当 dotted=false 时, try_consume(dotted) = try_consume('\0')
    // 在 handle_numeric 中消费完 "42" 后, 处于文件末尾
    // current() = '\0', try_consume('\0') 成功 → dotted=true
    // 然后继续读数字 → 没有更多数字
    // end=current_, str="42", stod("42")=42.0
    // 返回 double 而非 int!
    std::cout << "---\nKey insight: try_consume(dotted) with dotted=false\n";
    std::cout << "calls try_consume('\0') which succeeds at EOF!\n";
    std::cout << "This makes dotted=true, returning stod('42')=double\n";
}
