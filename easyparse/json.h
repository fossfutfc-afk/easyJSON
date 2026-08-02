#pragma once

#include <exception>
#include <stdexcept>
#include <variant>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <optional>

namespace json {
    class jsonException : public std::exception {
    public:
        explicit jsonException(std::string msg) : msg_(std::move(msg)) {}
        const char* what() const noexcept override { return msg_.c_str(); }
    private:
        std::string msg_;
    };
/*---------------------------------------------------------------------------------*/
    class jsonValue {
    public:
        using arr = std::vector<jsonValue>;
        using obj = std::unordered_map<std::string, jsonValue>;
        std::variant<std::monostate, int, double, std::string, bool, arr, obj> value;
    };
/*---------------------------------------------------------------------------------*/
    class jsonParser {
    public:
        jsonParser(std::string_view);
        jsonValue parse(); // prime-thod
        bool is_at_end() const; // loop control

    private:
        void whitespace();
        char read_escape();
        char advance();
        char current() const;
        void consume(const char, const std::string&);
        bool try_consume(const char);

        std::string handle_string();
        jsonValue::arr handle_array();
        jsonValue::obj handle_object();
        jsonValue handle_numeric();
        std::optional<jsonValue> handle_keyword();

    private:
        std::string file_;
        size_t current_;
    };
/*---------------------------------------------------------------------------------*/
    inline jsonParser::jsonParser(std::string_view file) : file_(file), current_(0){};

    inline jsonValue jsonParser::parse() {
        whitespace();

        const char c {current()};
        switch (c) {
            case '{':
                return jsonValue {handle_object()};
                break;
            case '[':
                return jsonValue {handle_array()};
                break;
            case '"':
                return jsonValue {handle_string()};
                break;
            default: {
                if (std::isdigit(c) || c == '-') 
                    return jsonValue {handle_numeric()};
                else if (auto keyword {handle_keyword()}; keyword.has_value())
                    return *keyword;
                else 
                    throw jsonException("Unexpected character.");
                return jsonValue {std::monostate()}; // Unreachable
            }
        }
    }

    inline bool jsonParser::is_at_end() const {
        return current_ >= file_.size();
    }

    inline void jsonParser::whitespace() {
        while (!is_at_end() && std::isspace(current()))
            advance();
    }

    inline char jsonParser::read_escape() {
        if (is_at_end())
            throw jsonException("Unexpected end of input in escape sequence.");
        const char esc = advance();
        switch (esc) {
            case '"':  return '"';
            case '\\': return '\\';
            case '/':  return '/';
            case 'b':  return '\b';
            case 'f':  return '\f';
            case 'n':  return '\n';
            case 'r':  return '\r';
            case 't':  return '\t';
            case 'u': {
                if (current_ + 4 > file_.size())
                    throw jsonException("Unexpected end of input in \\u escape.");
                const std::string hex = file_.substr(current_, 4);
                char* endp = nullptr;
                const unsigned long cp = std::strtoul(hex.c_str(), &endp, 16);
                if (endp != hex.c_str() + 4)
                    throw jsonException("Invalid \\u escape sequence.");
                current_ += 4;
                return static_cast<char>(cp);
            }
            default:
                throw jsonException(std::string("Invalid escape sequence: \\") + esc);
        }
    }

    inline char jsonParser::advance() {
        if (is_at_end())
            return '\0';
        return file_[current_++];
    }

    inline char jsonParser::current() const {
        if (is_at_end())
            return '\0';
        return file_[current_];
    }

    inline void jsonParser::consume(const char c, const std::string& msg) {
        if (current() != c)
            throw jsonException(msg);
        advance();
    }
    
    inline bool jsonParser::try_consume(const char c) {
        if (current() != c)
            return false;
        advance();
        return true;
    }

    inline std::string jsonParser::handle_string() {
        consume('"', "Expected an opening quotation mark.");

        std::string result;
        while (current() != '"') {
            if (is_at_end())
                throw jsonException("Unexpected end of input in string.");
            const char c = advance();
            if (c == '\\') {
                result += read_escape();
            } else if (c == '\r' || c == '\n') {
                throw jsonException("Unescaped newline in string.");
            } else {
                result += c;
            }
        }
        consume('"', "Expected a closing quotation mark.");
        return result;
    }

    inline jsonValue::arr jsonParser::handle_array() {
        jsonValue::arr output {};
        consume('[', "Expected an opening bracket.");
        whitespace();
        if (try_consume(']'))
            return output;
        do {
            whitespace();
            output.push_back(parse());
            whitespace();
            if (try_consume(']'))
                return output;
            if (!try_consume(','))
                throw jsonException("Expected ',' or ']' in array.");
            whitespace();
            if (current() == ']')
                throw jsonException("Trailing comma in array.");
        } while (true);
    }

    inline jsonValue::obj jsonParser::handle_object() {
        jsonValue::obj output {};
        consume('{', "Expected an opening bracket.");
        whitespace();
        if (try_consume('}'))
            return output;
        do {
            whitespace();
            const auto key {handle_string()};
            whitespace();
            consume(':', "Expected colon seperator between key and value.");
            whitespace();
            if (!output.insert({key, parse()}).second)
                throw jsonException("Duplicate key: \"" + key + "\".");
            whitespace();
            if (try_consume('}'))
                return output;
            if (!try_consume(','))
                throw jsonException("Expected ',' or '}' in object.");
            whitespace();
            if (current() == '}')
                throw jsonException("Trailing comma in object.");
        } while (true);
    }

    inline jsonValue jsonParser::handle_numeric() {
        bool dotted = false;
        const size_t start {current_};

        // consume leading minus
        try_consume('-');

        // integer part with leading-zero rejection
        // 0.1 and 0e1 is valid
        if (try_consume('0')) {
            if (!is_at_end() && std::isdigit(current()))
                throw jsonException("Leading zeros are not allowed.");
        } else {
            if (is_at_end() || !std::isdigit(current()))
                throw jsonException("Expected digit in number.");
            while (!is_at_end() && std::isdigit(current()))
                advance();
        }
        
        if (try_consume('.')) {
            dotted = true;
            if (is_at_end() || !std::isdigit(current()))
                throw jsonException("Expected digit after decimal point.");
            while (!is_at_end() && std::isdigit(current()))
                advance();
        }

        if (try_consume('e') || try_consume('E')) {
            dotted = true;
            try_consume('+') || try_consume('-');
            if (is_at_end() || !std::isdigit(current()))
                throw jsonException("Expected digit in exponent.");
            while (!is_at_end() && std::isdigit(current()))
                advance();
        }

        const size_t end {current_};

        const auto str {file_.substr(start, end - start)};
        if (dotted) {
            double d;
            try {
                d = std::stod(str);
            } catch (const std::exception&) {
                throw jsonException("Failed to parse number: " + str);
            }
            if (std::isnan(d) || std::isinf(d))
                throw jsonException("NaN and Infinity are not valid JSON numbers.");
            return {d};
        }
        try {
            return {std::stoi(str)};
        } catch (const std::exception&) {
            throw jsonException("Failed to parse number: " + str);
        }
    }

    inline std::optional<jsonValue> jsonParser::handle_keyword() {
        static const std::unordered_map<std::string, jsonValue> keyword_map {
            {"true", jsonValue{true}},
            {"false", jsonValue{false}},
            {"null", jsonValue{std::monostate()}}
        };

        for (const auto& [kw, val] : keyword_map) {
            const auto end {kw.size() + current_};
            if (end > file_.size()) 
                continue;
            if (file_.substr(current_, kw.size()) == kw) {
                current_ += kw.size();
                return val;
            }
        }
        return std::nullopt;
    }
}