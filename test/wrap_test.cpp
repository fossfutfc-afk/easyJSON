#include "../easyparse/json.h"
#include <iostream>

int main() {
    // 验证双重包装问题
    // handle_numeric() 返回 jsonValue，但 parse() 又用它构造 jsonValue
    
    // 直接用 handle_numeric 的结果（模拟修复后的行为）
    json::jsonParser p1("42");
    // 无法直接调用 private 方法，但我们可以去掉那层包装
    
    // 测试：jsonValue{jsonValue{42}} 会发生什么？
    json::jsonValue inner;
    inner.value = 42;
    
    // 尝试用 inner 来构造另一个 jsonValue
    json::jsonValue outer{inner};  // 这会编译吗？
    std::cout << "This compiles\n";
    
    // 检查 outer 的实际类型
    auto* arr = std::get_if<json::jsonValue::arr>(&outer.value);
    auto* obj = std::get_if<json::jsonValue::obj>(&outer.value);
    auto* i = std::get_if<int>(&outer.value);
    std::cout << "arr=" << (void*)arr << " obj=" << (void*)obj << " int=" << (void*)i << "\n";
}
