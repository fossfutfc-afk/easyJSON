#include "../easyparse/json.h"
#include <iostream>

void test(const char* label, const char* input) {
    try {
        json::jsonParser(input).parse();
        std::cout << label << ": OK (WARNING: should be rejected!)\n";
    } catch (const json::jsonException& e) {
        std::cout << label << ": REJECTED — " << e.what() << "\n";
    }
}

int main() {
    // Q1: 嵌套对象中同名key，是否误判？
    test("nested same key",
         "{\"person1\":{\"age\":8},\"person2\":{\"age\":9}}");

    // Q2: 尾随逗号
    test("trailing comma obj",
         "{\"a\":1,}");
    test("trailing comma arr",
         "[1,]");

    // 对比：正确的尾随逗号报错
    test("normal obj (control)",
         "{\"a\":1}");
    test("normal arr (control)",
         "[1]");
}
