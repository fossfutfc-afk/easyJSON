# easyJSON

Header-only JSON libraries for C++17. No dependencies beyond the standard library. Two flavours:

| File | Namespace | Description |
|---|---|---|
| `easyparse/json.h` | `json` | Minimal parser — ~300 lines, parse only |
| `easyparse/json_adv.h` | `json_adv` | Full-featured — serialization, Unicode, SAX streaming |

---

## easyparse/json.h — Lightweight parser

Drop-in, ~300 lines. Parses JSON text into a `std::variant` tree.

```cpp
#include "json.h"

json::jsonParser p(R"({"name":"easyparse","version":1})");
json::jsonValue v = p.parse();

auto& obj = std::get<json::jsonValue::obj>(v.value);
std::cout << std::get<std::string>(obj["name"].value); // "easyparse"
```

### API

| Type | Role |
|---|---|
| `json::jsonValue` | Variant tree: `monostate`/`int`/`double`/`string`/`bool`/`arr`/`obj` |
| `json::jsonParser` | `parse()` reads one value; `is_at_end()` controls loops |
| `json::jsonException` | `std::exception` subclass, `.what()` for details |

### Features

- Objects / arrays / strings / numbers / `true` / `false` / `null`
- Escape sequences: `\" \\ \/ \b \f \n \r \t \uXXXX`
- Scientific notation (`1.5e-3`), leading-zero rejection (`01` → error)
- Duplicate-key detection, trailing-comma rejection
- NaN/Infinity are rejected

---

## easyparse/json_adv.h — Full-featured

Same parsing core, plus serialization, full Unicode (surrogate pairs), SAX streaming, and numeric type promotion.

```cpp
#include "json_adv.h"
using namespace json_adv;

// --- Parse ---
jsonValue v = jsonParser(R"({"items":[1,2,3]})").parse();

// --- Serialize (compact) ---
std::string s = to_string(v);
// → {"items":[1,2,3]}

// --- Serialize (pretty-print) ---
std::string s2 = to_string(v, Indent::TwoSpaces);
// → {
//     "items": [
//       1,
//       2,
//       3
//     ]
//   }

// --- SAX streaming ---
struct MyHandler : SaxHandler {
    void on_int(int v)              override { /* ... */ }
    void on_string(const std::string& v) override { /* ... */ }
    void on_begin_object()          override { /* ... */ }
    void on_key(const std::string& k) override { /* ... */ }
    void on_end_object()            override { /* ... */ }
    // ... all 10 events
};
MyHandler h;
SaxParser(R"({"a":1})", h).parse();
```

### API

Everything in `json_adv` plus:

| Symbol | Role |
|---|---|
| `jsonValue` | Variant: `monostate`/`int`/`long long`/`double`/`long double`/`string`/`bool`/`arr`/`obj` |
| `jsonParser` | Same as lightweight, with surrogate-pair-aware `\u` |
| `to_string(v, indent?)` | Serialize to JSON string (compact or pretty) |
| `encode_utf8(cp)` | Codepoint → UTF-8 bytes |
| `SaxHandler` | Abstract callback interface (10 events) |
| `SaxParser(sv, handler)` | Streaming parser, fires callbacks |

### What json_adv adds over json.h

- **Serialization** — `to_string()` with compact / pretty-print modes
- **Full Unicode** — surrogate pair support (`😀` → 😀), `encode_utf8()`
- **Numeric promotion** — `int` → `long long` → `double` → `long double` on overflow
- **SAX streaming** — callback-driven parser for large files, zero tree allocation

### SaxHandler events

| Method | Fires on |
|---|---|
| `on_null()` | `null` |
| `on_bool(bool)` | `true` / `false` |
| `on_int(int)` | integer number |
| `on_double(double)` | floating-point number |
| `on_string(string)` | string value |
| `on_begin_object()` | `{` |
| `on_key(string)` | object key |
| `on_end_object()` | `}` |
| `on_begin_array()` | `[` |
| `on_end_array()` | `]` |

## Strictness

Both parsers follow strict JSON. They do **not** support:

- Trailing commas (rejected with a specific error message)
- Comments (`//` or `/* */`)
- Unquoted keys
- Single-quoted strings
- `NaN` / `Infinity` as literals (serialization outputs `null` for these)

## Requirements

- **C++17** or later
- Standard library only — no external dependencies

## Other

A learning project inspired by [nlohmann/json](https://github.com/nlohmann/json). Use it, change it, copy it anywhere.
