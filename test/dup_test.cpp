#include "../easyparse/json.h"
#include <iostream>

int main() {
    // 重复key应该被拒绝
    try {
        json::jsonParser("{\"a\":1,\"a\":2}").parse();
        std::cout << "FAIL: duplicate key accepted\n";
        return 1;
    } catch (const json::jsonException& e) {
        std::cout << "PASS: " << e.what() << "\n";
    }

    // 不同key正常通过
    try {
        json::jsonParser("{\"a\":1,\"b\":2}").parse();
        std::cout << "PASS: distinct keys ok\n";
    } catch (const json::jsonException& e) {
        std::cout << "FAIL: " << e.what() << "\n";
        return 1;
    }

    // 空对象
    try {
        json::jsonParser("{}").parse();
        std::cout << "PASS: empty object ok\n";
    } catch (const json::jsonException& e) {
        std::cout << "FAIL: " << e.what() << "\n";
        return 1;
    }
}
