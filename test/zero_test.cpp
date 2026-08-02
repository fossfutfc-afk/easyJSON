#include "../easyparse/json.h"
#include <iostream>

void test(const char* name, const char* input) {
    try {
        auto v = json::jsonParser(input).parse();
        auto* d = std::get_if<double>(&v.value);
        auto* i = std::get_if<int>(&v.value);
        std::cout << name << " (" << input << "): OK → ";
        if (d) std::cout << "double " << *d << "\n";
        else if (i) std::cout << "int " << *i << "\n";
        else std::cout << "other\n";
    } catch (const json::jsonException& e) {
        std::cout << name << " (" << input << "): REJECTED — " << e.what() << "\n";
    }
}

int main() {
    test("0.123", "0.123");
    test("0e10",  "0e10");
    test("0123",  "0123");
    test("0",     "0");
    test("0.0",   "0.0");
}
