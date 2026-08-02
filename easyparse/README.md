# easyparse

A minimal, header-only JSON parser for C++17 and later. No dependencies beyond the standard library — just drop [json.h](json.h) into your project and go.

No serialization, no deserialization, no streaming parsing, only a parse method, which should be enough for toy projects...I guess so?

## Quick start

```cpp
#include "json.h"
#include <iostream>

int main() {
    std::string_view src = R"({
        "name": "easyparse",
        "version": 1,
        "dependencies": [],
        "strict": true,
        "metadata": null
    })";

    json::jsonParser parser(src);
    json::jsonValue result = parser.parse();

    // Access parsed data via std::visit or std::get
    auto& obj = std::get<json::jsonValue::obj>(result.value);
    std::cout << std::get<std::string>(obj["name"].value) << '\n';
    // prints: easyparse
}
```

### Parsing multiple values

`parse()` consumes one value at a time and leaves the cursor after it. For multi-value input, loop:

```cpp
json::jsonParser parser(src);
while (!parser.is_at_end()) {
    parser.whitespace();          // skip whitespace between values
    json::jsonValue val = parser.parse();
    // process val
}
```

## API

### `json::jsonValue`

The parsed JSON tree. Values are stored in a `std::variant`:

| C++ type | JSON type |
|---|---|
| `std::monostate` | `null` |
| `int` | number (integer) |
| `double` | number (float) |
| `std::string` | string |
| `bool` | `true` / `false` |
| `std::vector<jsonValue>` | array |
| `std::unordered_map<std::string, jsonValue>` | object |

Convenience aliases:

- `jsonValue::arr` — `std::vector<jsonValue>`
- `jsonValue::obj` — `std::unordered_map<std::string, jsonValue>`

### `json::jsonParser`

```cpp
explicit jsonParser(std::string_view src);
jsonValue parse();
```

- **Constructor** — takes the JSON source as a `std::string_view`. The source must outlive the parser (it is not copied).
- **`parse()`** — parses exactly **one** JSON value from the current position and returns it as a `json::jsonValue`. Throws `json::jsonException` on any error. To parse multiple values (e.g. a JSON Lines file), call `parse()` in a loop.

### `json::jsonException`

Inherits from `std::exception`. Call `.what()` for a human-readable error message.

## What it handles

- **Objects** `{}` — including duplicate-key detection and trailing-comma rejection
- **Arrays** `[]` — including trailing-comma rejection
- **Strings** — full escape-sequence support: `\"`, `\\`, `\/`, `\b`, `\f`, `\n`, `\r`, `\t`, and `\uXXXX`
- **Numbers** — integers (`int`) and floats (`double`), with:
  - Leading-zero rejection (`01` is invalid, `0.1` is fine)
  - Scientific notation (`1e10`, `1.5E-3`)
  - `NaN` and `Infinity` are rejected (not valid JSON)
- **Keywords** — `true`, `false`, `null`

## Strictness

This parser follows strict JSON — it intentionally does **not** support:

- Trailing commas
- Comments (`//` or `/* */`)
- Unquoted keys
- Single-quoted strings
- `NaN` / `Infinity` as numeric literals

## Requirements

- **C++17** or later (for `std::variant`, `std::string_view`, `std::optional`, structured bindings)
- No external dependencies — standard library only

## Other

- It's a learning project based on nlohmann's youtube instructions and I'm so grateful to him for offering such wonderful instructions.
- Feel free to change anything, copy paste it anywhere you want.
