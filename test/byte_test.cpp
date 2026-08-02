#include "../easyparse/json.h"
#include <cstdio>
#include <cstring>

int main() {
    const char* input = "\"x\"";
    printf("strlen=%zu  bytes: ", strlen(input));
    for (int i = 0; input[i]; i++) printf("%02x ", (unsigned char)input[i]);
    printf("\nfirst char='%c' (0x%02x)\n", input[0], (unsigned char)input[0]);

    try {
        json::jsonParser(input).parse();
        printf("OK\n");
    } catch (const std::exception& e) {
        printf("FAIL: %s\n", e.what());
    }
}
