#include "../easyparse/json.h"
#include <iostream>

int main() {
    json::jsonParser p("[1] [2,3] \"hello\" {\"k\":true}");

    int count = 0;
    while (!p.is_at_end()) {
        auto v = p.parse();
        ++count;
    }
    std::cout << "Parsed " << count << " values (expected 4)\n";
    if (count == 4) std::cout << "PASS\n"; else std::cout << "FAIL\n";
}
