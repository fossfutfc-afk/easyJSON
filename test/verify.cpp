#include <iostream>

// 检查 json.h 的实际内容
int main() {
    std::cout << "Testing direct parse...\n";
    
    // 手动模拟修复后的逻辑
    std::string file_ = "\"x\"";
    size_t current_ = 0;
    
    auto is_at_end = [&]() { return current_ >= file_.size(); };
    auto current = [&]() -> char { 
        if (is_at_end()) return '\0';
        return file_[current_];
    };
    auto advance = [&]() -> char {
        if (is_at_end()) return '\0';
        return file_[current_++];
    };
    auto try_consume = [&](char c) -> bool {
        if (current() != c) return false;
        advance();
        return true;
    };
    auto consume = [&](char c, const char* msg) {
        if (current() != c) {
            std::cout << "CONSUME FAIL: expected '" << c << "' at pos " << current_ << ", got '" << current() << "'\n";
            throw std::runtime_error(msg);
        }
        advance();
    };
    
    // parse string
    try {
        consume('"', "open quote");
        while (current() != '"') {
            advance();
            if (is_at_end()) break;
        }
        consume('"', "close quote");
        std::cout << "Manual parse OK\n";
    } catch (const std::exception& e) {
        std::cout << "Manual parse FAIL: " << e.what() << "\n";
    }
    
    // Now test number
    file_ = "42";
    current_ = 0;
    try {
        bool dotted = false;
        size_t start = current_;
        while (isdigit(current()) && !is_at_end()) advance();
        std::cout << "After digits: pos=" << current_ << ", current()='" << current() << "'\n";
        
        bool consumed = try_consume(dotted);  // try_consume('\0')
        std::cout << "try_consume(dotted=false) at pos " << current_ << ": " << (consumed ? "true" : "false") << "\n";
        std::cout << "After try_consume: pos=" << current_ << "\n";
        
        if (consumed) {
            dotted = true;
            std::cout << "dotted set to true!\n";
        }
        size_t end = current_;
        auto str = file_.substr(start, end - start);
        std::cout << "str='" << str << "' dotted=" << dotted << "\n";
    } catch (const std::exception& e) {
        std::cout << "FAIL: " << e.what() << "\n";
    }
}
