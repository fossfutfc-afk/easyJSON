#include "../easyparse/json.h"
#include <iostream>

void test(const char* label, const char* input) {
    try {
        auto v = json::jsonParser(input).parse();
        auto* d = std::get_if<double>(&v.value);
        std::cout << label << ": OK → " << (d ? std::to_string(*d) : "not double") << "\n";
    } catch (const json::jsonException& e) {
        std::cout << label << ": REJECTED — " << e.what() << "\n";
    }
}

int main() {
    test("1e999 (overflow)", "1e999");
    test("-1e999", "-1e999");
    test("1.5", "1.5");
    test("正常浮点", "3.14");
}
