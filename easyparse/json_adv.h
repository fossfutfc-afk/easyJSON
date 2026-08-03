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
#include <limits>
#include <optional>
#include <functional>
#include <sstream>
#include <iomanip>

namespace json_adv {

// ============================================================
//  jsonException
// ============================================================

class jsonException : public std::exception {
public:
    explicit jsonException(std::string msg) : msg_(std::move(msg)) {}
    const char* what() const noexcept override { return msg_.c_str(); }
private:
    std::string msg_;
};

// ============================================================
//  jsonValue
// ============================================================

class jsonValue {
public:
    using arr = std::vector<jsonValue>;
    using obj = std::unordered_map<std::string, jsonValue>;
    std::variant<std::monostate, int, long long, double, long double, std::string, bool, arr, obj> value;

    jsonValue() = default;
    jsonValue(std::monostate v) : value(v) {}
    jsonValue(int v)           : value(v) {}
    jsonValue(long long v)     : value(v) {}
    jsonValue(double v)        : value(v) {}
    jsonValue(long double v)   : value(v) {}
    jsonValue(std::string v)   : value(std::move(v)) {}
    jsonValue(bool v)          : value(v) {}
    jsonValue(arr v)           : value(std::move(v)) {}
    jsonValue(obj v)           : value(std::move(v)) {}
};

// ============================================================
//  Unicode helpers
// ============================================================

inline std::string encode_utf8(char32_t cp)
{
    if (cp > 0x10FFFF)
        throw jsonException("Codepoint out of Unicode range.");

    // surrogate halves are illegal on their own
    if (cp >= 0xD800 && cp <= 0xDFFF)
        throw jsonException("Lone surrogate half is not a valid codepoint.");

    std::string out;
    if (cp <= 0x7F) {
        out += static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
        out += static_cast<char>(0xE0 |  (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 |  (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 |  (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 |  (cp & 0x3F));
    }
    return out;
}

// ============================================================
//  jsonParser
// ============================================================

class jsonParser {
public:
    explicit jsonParser(std::string_view sv);
    jsonValue parse();
    bool is_at_end() const;

private:
    // --- character layer ---
    void whitespace();
    char advance();
    char current() const;
    void consume(const char c, const std::string& msg);
    bool try_consume(const char c);

    // --- unicode ---
    std::string read_escape();          // returns UTF-8 bytes (1–4 per escape)
    char32_t read_codepoint();          // reads \uXXXX, handles surrogate pairs

    // --- handlers ---
    std::string handle_string();
    jsonValue::arr handle_array();
    jsonValue::obj handle_object();
    jsonValue handle_numeric();
    std::optional<jsonValue> handle_keyword();

    std::string file_;
    size_t current_;
};

// ============================================================
//  jsonParser implementation
// ============================================================

inline jsonParser::jsonParser(std::string_view file)
    : file_(file), current_(0) {}

// --- character layer ---

inline bool jsonParser::is_at_end() const {
    return current_ >= file_.size();
}

inline void jsonParser::whitespace() {
    while (!is_at_end() && std::isspace(current()))
        advance();
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

// --- unicode ---

// Reads a single \uXXXX escape (4 hex digits).
// If the result is a high surrogate (0xD800–0xDBFF), reads the immediately
// following \uXXXX low surrogate and combines them into one codepoint.
inline char32_t jsonParser::read_codepoint()
{
    if (current_ + 4 > file_.size())
        throw jsonException("Unexpected end of input in \\u escape.");

    const std::string hex = file_.substr(current_, 4);
    char* endp = nullptr;
    unsigned long cp = std::strtoul(hex.c_str(), &endp, 16);
    if (endp != hex.c_str() + 4)
        throw jsonException("Invalid \\u escape sequence.");
    current_ += 4;

    // high surrogate — expect a low surrogate pair
    if (cp >= 0xD800 && cp <= 0xDBFF) {
        // require exactly \uXXXX
        if (current_ + 6 > file_.size() ||
            file_[current_] != '\\' || file_[current_ + 1] != 'u')
            throw jsonException("Expected low surrogate after \\u high surrogate.");

        current_ += 2; // skip \u
        if (current_ + 4 > file_.size())
            throw jsonException("Unexpected end of input in low surrogate.");

        const std::string lo_hex = file_.substr(current_, 4);
        unsigned long lo = std::strtoul(lo_hex.c_str(), &endp, 16);
        if (endp != lo_hex.c_str() + 4)
            throw jsonException("Invalid low surrogate escape.");
        current_ += 4;

        if (lo < 0xDC00 || lo > 0xDFFF)
            throw jsonException("Invalid low surrogate value.");

        cp = 0x10000 + (cp - 0xD800) * 0x400 + (lo - 0xDC00);
    }
    // lone low surrogate
    else if (cp >= 0xDC00 && cp <= 0xDFFF) {
        throw jsonException("Unexpected low surrogate without high surrogate.");
    }

    return static_cast<char32_t>(cp);
}

inline std::string jsonParser::read_escape()
{
    if (is_at_end())
        throw jsonException("Unexpected end of input in escape sequence.");

    const char esc = advance();
    switch (esc) {
        case '"':  return "\"";
        case '\\': return "\\";
        case '/':  return "/";
        case 'b':  return "\b";
        case 'f':  return "\f";
        case 'n':  return "\n";
        case 'r':  return "\r";
        case 't':  return "\t";
        case 'u':  return encode_utf8(read_codepoint());
        default:
            throw jsonException(std::string("Invalid escape sequence: \\") + esc);
    }
}

// --- main dispatch ---

inline jsonValue jsonParser::parse()
{
    whitespace();

    const char c {current()};
    switch (c) {
        case '{':
            return jsonValue {handle_object()};
        case '[':
            return jsonValue {handle_array()};
        case '"':
            return jsonValue {handle_string()};
        default: {
            if (std::isdigit(c) || c == '-')
                return handle_numeric();
            if (auto keyword {handle_keyword()}; keyword.has_value())
                return *keyword;
            throw jsonException("Unexpected character.");
        }
    }
}

// --- handlers ---

inline std::string jsonParser::handle_string()
{
    consume('"', "Expected an opening quotation mark.");

    std::string result;
    while (current() != '"') {
        if (is_at_end())
            throw jsonException("Unexpected end of input in string.");
        const char c = advance();
        if (c == '\\') {
            result += read_escape();          // append UTF-8 bytes
        } else if (c == '\r' || c == '\n') {
            throw jsonException("Unescaped newline in string.");
        } else {
            result += c;
        }
    }
    consume('"', "Expected a closing quotation mark.");
    return result;
}

inline jsonValue::arr jsonParser::handle_array()
{
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

inline jsonValue::obj jsonParser::handle_object()
{
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

inline jsonValue jsonParser::handle_numeric()
{
    bool dotted = false;
    const size_t start {current_};

    try_consume('-');

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
        bool overflow = false;
        try {
            d = std::stod(str);
        } catch (const std::exception&) {
            overflow = true;  // MinGW throws on overflow instead of returning Inf
        }
        if (!overflow && !std::isnan(d) && !std::isinf(d))
            return jsonValue{d};
        if (!overflow && std::isnan(d))
            throw jsonException("NaN is not a valid JSON number.");

        // double can't hold it → promote to long double
        long double ld;
        try {
            ld = std::stold(str);
        } catch (const std::exception&) {
            throw jsonException("Number out of range: " + str);
        }
        if (std::isnan(ld) || std::isinf(ld))
            throw jsonException("Number out of range: " + str);
        return jsonValue{ld};
    }

    // integer path — promote on overflow: int → long long → double → long double
    try {
        return jsonValue{std::stoi(str)};
    } catch (const std::out_of_range&) {
        try {
            return jsonValue{std::stoll(str)};
        } catch (const std::out_of_range&) {
            double d;
            bool overflow = false;
            try {
                d = std::stod(str);
            } catch (const std::exception&) {
                overflow = true;
            }
            if (!overflow && !std::isnan(d) && !std::isinf(d))
                return jsonValue{d};

            // double can't hold it → promote to long double
            long double ld;
            try {
                ld = std::stold(str);
            } catch (const std::exception&) {
                throw jsonException("Integer too large: " + str);
            }
            if (std::isnan(ld) || std::isinf(ld))
                throw jsonException("Integer too large: " + str);
            return jsonValue{ld};
        }
    } catch (const std::exception&) {
        throw jsonException("Failed to parse number: " + str);
    }
}

inline std::optional<jsonValue> jsonParser::handle_keyword()
{
    static const std::unordered_map<std::string, jsonValue> keyword_map {
        {"true",  jsonValue{true}},
        {"false", jsonValue{false}},
        {"null",  jsonValue{std::monostate{}}}
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

// ============================================================
//  Serialization
// ============================================================

enum class Indent { None, TwoSpaces, FourSpaces, Tab };

namespace impl {

inline const char* indent_unit(Indent ind) {
    switch (ind) {
        case Indent::TwoSpaces:  return "  ";
        case Indent::FourSpaces: return "    ";
        case Indent::Tab:        return "\t";
        default:                 return "";
    }
}

inline std::string make_indent(Indent ind, int level) {
    if (ind == Indent::None) return "";
    const char* unit = indent_unit(ind);
    std::string s;
    for (int i = 0; i < level; ++i) s += unit;
    return s;
}

inline void write_string(std::ostringstream& oss, const std::string& s) {
    oss << '"';
    for (char c : s) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b";  break;
            case '\f': oss << "\\f";  break;
            case '\n': oss << "\\n";  break;
            case '\r': oss << "\\r";  break;
            case '\t': oss << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                    oss << "\\u" << std::hex << std::setw(4)
                        << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(c))
                        << std::dec;
                else
                    oss << c;
        }
    }
    oss << '"';
}

inline void serialize(std::ostringstream& oss, const jsonValue& v,
                      Indent indent, int level)
{
    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::monostate>) {
            oss << "null";
        }
        else if constexpr (std::is_same_v<T, int>) {
            oss << arg;
        }
        else if constexpr (std::is_same_v<T, long long>) {
            oss << arg;
        }
        else if constexpr (std::is_same_v<T, double>) {
            if (std::isnan(arg) || std::isinf(arg))
                oss << "null";
            else
                oss << std::setprecision(
                           std::numeric_limits<double>::max_digits10) << arg;
        }
        else if constexpr (std::is_same_v<T, long double>) {
            if (std::isnan(arg) || std::isinf(arg))
                oss << "null";
            else
                oss << std::setprecision(
                           std::numeric_limits<long double>::max_digits10) << arg;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            oss << (arg ? "true" : "false");
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            write_string(oss, arg);
        }
        else if constexpr (std::is_same_v<T, jsonValue::arr>) {
            oss << '[';
            if (!arg.empty()) {
                const bool pretty = (indent != Indent::None);
                const std::string inner_indent = make_indent(indent, level + 1);
                const std::string close_indent = make_indent(indent, level);

                if (pretty) oss << '\n';
                for (size_t i = 0; i < arg.size(); ++i) {
                    if (i > 0) {
                        oss << ',';
                        if (pretty) oss << '\n';
                    }
                    if (pretty) oss << inner_indent;
                    serialize(oss, arg[i], indent, level + 1);
                }
                if (pretty) {
                    oss << '\n';
                    oss << close_indent;
                }
            }
            oss << ']';
        }
        else if constexpr (std::is_same_v<T, jsonValue::obj>) {
            oss << '{';
            if (!arg.empty()) {
                const bool pretty = (indent != Indent::None);
                const std::string inner_indent = make_indent(indent, level + 1);
                const std::string close_indent = make_indent(indent, level);
                size_t i = 0;

                if (pretty) oss << '\n';
                for (const auto& [key, val] : arg) {
                    if (i++ > 0) {
                        oss << ',';
                        if (pretty) oss << '\n';
                    }
                    if (pretty) oss << inner_indent;
                    write_string(oss, key);
                    oss << ':';
                    if (pretty) oss << ' ';
                    serialize(oss, val, indent, level + 1);
                }
                if (pretty) {
                    oss << '\n';
                    oss << close_indent;
                }
            }
            oss << '}';
        }
    }, v.value);
}

} // namespace impl

inline std::string to_string(const jsonValue& v, Indent indent = Indent::None) {
    std::ostringstream oss;
    impl::serialize(oss, v, indent, 0);
    return oss.str();
}

// ============================================================
//  Streaming / SAX parser
// ============================================================

struct SaxHandler {
    virtual void on_null()                         = 0;
    virtual void on_bool(bool v)                   = 0;
    virtual void on_int(int v)                     = 0;
    virtual void on_double(double v)               = 0;
    virtual void on_string(const std::string& v)   = 0;
    virtual void on_begin_object()                 = 0;
    virtual void on_key(const std::string& key)    = 0;
    virtual void on_end_object()                   = 0;
    virtual void on_begin_array()                  = 0;
    virtual void on_end_array()                    = 0;
    virtual ~SaxHandler() = default;
};

class SaxParser {
public:
    SaxParser(std::string_view sv, SaxHandler& handler);
    void parse();

private:
    // --- character layer ---
    void skip_whitespace();
    char advance();
    char peek() const;
    void expect(char c, const std::string& msg);
    bool try_consume(char c);
    bool at_end() const;

    // --- unicode ---
    std::string read_string();
    std::string read_escape();
    char32_t read_codepoint();

    // --- dispatch ---
    void parse_value();
    void parse_array();
    void parse_object();
    void parse_number();
    void parse_keyword();

    std::string src_;
    size_t pos_;
    SaxHandler& handler_;
};

// --- character layer ---

inline SaxParser::SaxParser(std::string_view sv, SaxHandler& handler)
    : src_(sv), pos_(0), handler_(handler) {}

inline bool SaxParser::at_end() const {
    return pos_ >= src_.size();
}

inline char SaxParser::peek() const {
    if (at_end()) return '\0';
    return src_[pos_];
}

inline char SaxParser::advance() {
    if (at_end()) return '\0';
    return src_[pos_++];
}

inline void SaxParser::skip_whitespace() {
    while (!at_end() && std::isspace(peek()))
        advance();
}

inline void SaxParser::expect(char c, const std::string& msg) {
    if (peek() != c)
        throw jsonException(msg);
    advance();
}

inline bool SaxParser::try_consume(char c) {
    if (peek() != c)
        return false;
    advance();
    return true;
}

// --- unicode ---

inline char32_t SaxParser::read_codepoint()
{
    if (pos_ + 4 > src_.size())
        throw jsonException("Unexpected end of input in \\u escape.");

    const std::string hex = src_.substr(pos_, 4);
    char* endp = nullptr;
    unsigned long cp = std::strtoul(hex.c_str(), &endp, 16);
    if (endp != hex.c_str() + 4)
        throw jsonException("Invalid \\u escape sequence.");
    pos_ += 4;

    if (cp >= 0xD800 && cp <= 0xDBFF) {
        if (pos_ + 6 > src_.size() || src_[pos_] != '\\' || src_[pos_ + 1] != 'u')
            throw jsonException("Expected low surrogate after \\u high surrogate.");
        pos_ += 2;
        if (pos_ + 4 > src_.size())
            throw jsonException("Unexpected end of input in low surrogate.");
        const std::string lo_hex = src_.substr(pos_, 4);
        unsigned long lo = std::strtoul(lo_hex.c_str(), &endp, 16);
        if (endp != lo_hex.c_str() + 4)
            throw jsonException("Invalid low surrogate escape.");
        pos_ += 4;
        if (lo < 0xDC00 || lo > 0xDFFF)
            throw jsonException("Invalid low surrogate value.");
        cp = 0x10000 + (cp - 0xD800) * 0x400 + (lo - 0xDC00);
    }
    else if (cp >= 0xDC00 && cp <= 0xDFFF) {
        throw jsonException("Unexpected low surrogate without high surrogate.");
    }
    return static_cast<char32_t>(cp);
}

inline std::string SaxParser::read_escape()
{
    if (at_end())
        throw jsonException("Unexpected end of input in escape sequence.");
    const char esc = advance();
    switch (esc) {
        case '"':  return "\"";
        case '\\': return "\\";
        case '/':  return "/";
        case 'b':  return "\b";
        case 'f':  return "\f";
        case 'n':  return "\n";
        case 'r':  return "\r";
        case 't':  return "\t";
        case 'u':  return encode_utf8(read_codepoint());
        default:
            throw jsonException(std::string("Invalid escape sequence: \\") + esc);
    }
}

inline std::string SaxParser::read_string()
{
    expect('"', "Expected an opening quotation mark.");
    std::string result;
    while (peek() != '"') {
        if (at_end())
            throw jsonException("Unexpected end of input in string.");
        const char c = advance();
        if (c == '\\')
            result += read_escape();
        else if (c == '\r' || c == '\n')
            throw jsonException("Unescaped newline in string.");
        else
            result += c;
    }
    expect('"', "Expected a closing quotation mark.");
    return result;
}

// --- dispatch ---

inline void SaxParser::parse()
{
    skip_whitespace();
    if (at_end())
        throw jsonException("Unexpected end of input.");
    parse_value();
    skip_whitespace();
}

inline void SaxParser::parse_value()
{
    skip_whitespace();
    if (at_end())
        throw jsonException("Unexpected end of input.");

    const char c = peek();
    switch (c) {
        case '{':  return parse_object();
        case '[':  return parse_array();
        case '"':  handler_.on_string(read_string()); break;
        default:
            if (std::isdigit(c) || c == '-')
                return parse_number();
            return parse_keyword();
    }
}

inline void SaxParser::parse_array()
{
    handler_.on_begin_array();
    expect('[', "Expected an opening bracket.");
    skip_whitespace();
    if (try_consume(']')) {
        handler_.on_end_array();
        return;
    }
    do {
        skip_whitespace();
        parse_value();
        skip_whitespace();
        if (try_consume(']')) {
            handler_.on_end_array();
            return;
        }
        if (!try_consume(','))
            throw jsonException("Expected ',' or ']' in array.");
        skip_whitespace();
        if (peek() == ']')
            throw jsonException("Trailing comma in array.");
    } while (true);
}

inline void SaxParser::parse_object()
{
    handler_.on_begin_object();
    expect('{', "Expected an opening bracket.");
    skip_whitespace();
    if (try_consume('}')) {
        handler_.on_end_object();
        return;
    }
    do {
        skip_whitespace();
        const auto key = read_string();
        handler_.on_key(key);
        skip_whitespace();
        expect(':', "Expected colon separator between key and value.");
        parse_value();
        skip_whitespace();
        if (try_consume('}')) {
            handler_.on_end_object();
            return;
        }
        if (!try_consume(','))
            throw jsonException("Expected ',' or '}' in object.");
        skip_whitespace();
        if (peek() == '}')
            throw jsonException("Trailing comma in object.");
    } while (true);
}

inline void SaxParser::parse_number()
{
    bool dotted = false;
    const size_t start = pos_;

    try_consume('-');

    if (try_consume('0')) {
        if (!at_end() && std::isdigit(peek()))
            throw jsonException("Leading zeros are not allowed.");
    } else {
        if (at_end() || !std::isdigit(peek()))
            throw jsonException("Expected digit in number.");
        while (!at_end() && std::isdigit(peek()))
            advance();
    }

    if (try_consume('.')) {
        dotted = true;
        if (at_end() || !std::isdigit(peek()))
            throw jsonException("Expected digit after decimal point.");
        while (!at_end() && std::isdigit(peek()))
            advance();
    }

    if (try_consume('e') || try_consume('E')) {
        dotted = true;
        try_consume('+') || try_consume('-');
        if (at_end() || !std::isdigit(peek()))
            throw jsonException("Expected digit in exponent.");
        while (!at_end() && std::isdigit(peek()))
            advance();
    }

    const size_t end = pos_;
    const auto num_str = src_.substr(start, end - start);

    if (dotted) {
        double d;
        try { d = std::stod(num_str); }
        catch (const std::exception&) { throw jsonException("Failed to parse number: " + num_str); }
        if (std::isnan(d)) throw jsonException("NaN is not a valid JSON number.");
        if (std::isinf(d)) throw jsonException("Number out of range: " + num_str);
        handler_.on_double(d);
        return;
    }

    try {
        handler_.on_int(std::stoi(num_str));
        return;
    } catch (const std::out_of_range&) {}

    // int overflow → promote to double
    double d = std::stod(num_str);
    if (std::isnan(d) || std::isinf(d))
        throw jsonException("Integer too large: " + num_str);
    handler_.on_double(d);
}

inline void SaxParser::parse_keyword()
{
    static const std::unordered_map<std::string, std::function<void(SaxHandler&)>> kw_map {
        {"true",  [](SaxHandler& h) { h.on_bool(true); }},
        {"false", [](SaxHandler& h) { h.on_bool(false); }},
        {"null",  [](SaxHandler& h) { h.on_null(); }}
    };

    for (const auto& [kw, action] : kw_map) {
        if (pos_ + kw.size() > src_.size())
            continue;
        if (src_.substr(pos_, kw.size()) == kw) {
            pos_ += kw.size();
            action(handler_);
            return;
        }
    }
    throw jsonException("Unexpected character.");
}

} // namespace json_adv
