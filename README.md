# ase-types

[![Layer](https://img.shields.io/badge/Layer-0%20Foundation-blue.svg)]()
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)]()
[![Header Only](https://img.shields.io/badge/Header-Only-green.svg)]()

> Rust-inspired type-safe error handling and optional values for C++

Part of [ASE - Antares Simulation Engine](../../..)

## Overview

`ase-types` provides Rust-style `Option<T>` and `Result<T, E>` types for expressive, type-safe error handling without exceptions, replacing the C++ patterns of returning sentinel values or throwing exceptions that ECS systems cannot safely use. Option<T> replaces std::optional (which is forbidden in ECS components) with a type that enforces explicit null checks at compile time — accessing an empty Option without checking is_some() is a compile error, not a runtime crash. Result<T, E> replaces exception-based error handling with explicit success/failure returns that calling code must handle, making error paths visible in the type signature rather than hidden in try-catch blocks. These types are used across the entire engine: database queries return Result, configuration lookups return Option, and system calculations that might fail return Result with typed error codes. The module also defines the InvalidEntityId constant (UINT32_MAX) used in all components for entity references, and common type aliases used in component definitions. As a Layer 0 foundation library, ase-types has no ASE dependencies and is imported by every module that defines components.

## Features

- **Option<T>**: Type-safe optional values (replaces raw pointers and `std::optional` with better API)
- **Result<T, E>**: Type-safe error handling (replaces exceptions and error codes)
- **Functional API**: `map`, `and_then`, `filter`, `unwrap_or`, `unwrap_or_else`
- **No Exceptions**: Zero-cost abstractions for error handling without exception overhead
- **Rust Semantics**: Familiar API for developers coming from Rust

## Installation

```cmake
# Add to your CMakeLists.txt
add_subdirectory(foundation/ase-types)
target_link_libraries(your_target PRIVATE ase-types)
```

Header-only library - include what you need:

```cpp
#include <ase/types/option.hpp>
#include <ase/types/result.hpp>
#include <ase/types/types.hpp>  // Includes both Option and Result
```

## Usage

### Option<T> - Optional Values

Replace nullable pointers and `std::optional` with explicit handling:

```cpp
using namespace ase::types;

// Creation
Option<int> some_value = some(42);
Option<int> no_value = none<int>();

// Pattern matching
if (some_value.is_some()) {
    int val = some_value.unwrap();  // 42
}

// Safe unwrap with default
int result = no_value.unwrap_or(0);  // 0

// Functional transformations
auto doubled = some_value.map<int>([](int x) { return x * 2; });  // Some(84)
auto nothing = no_value.map<int>([](int x) { return x * 2; });    // None

// Chaining
auto chain = some(10)
    .map<int>([](int x) { return x + 5; })       // Some(15)
    .filter([](int x) { return x > 12; })        // Some(15) (passes filter)
    .map<int>([](int x) { return x * 2; });      // Some(30)

// Unwrap or compute default
auto computed = no_value.unwrap_or_else([]() { return expensive_computation(); });
```

### Result<T, E> - Error Handling

Replace exceptions and error codes with explicit error types:

```cpp
using namespace ase::types;

// Function that can fail
Result<int, std::string> divide(int a, int b) {
    if (b == 0) {
        return Result<int, std::string>::err("Division by zero");
    }
    return Result<int, std::string>::ok(a / b);
}

// Pattern matching
auto result = divide(10, 2);
if (result.is_ok()) {
    int value = result.unwrap();  // 5
} else {
    std::string error = result.unwrap_err();  // Won't execute
}

// Safe unwrap with default
int safe_result = divide(10, 0).unwrap_or(0);  // 0 (because division failed)

// Functional transformations
auto doubled = divide(10, 2)
    .map<int>([](int x) { return x * 2; });  // Ok(10)

auto failed = divide(10, 0)
    .map<int>([](int x) { return x * 2; });  // Err("Division by zero")

// Chaining operations
auto chain = divide(10, 2)
    .and_then<int>([](int x) { return divide(x, 5); })  // Ok(1)
    .map<int>([](int x) { return x + 100; });           // Ok(101)

// Error transformation
auto mapped_err = divide(10, 0)
    .map_err<int>([](const std::string& e) { return e.length(); });  // Err(17)
```

### SimpleResult - Common Pattern

For simple error messages, use the `SimpleResult<T>` alias:

```cpp
using namespace ase::types;

// SimpleResult<T> = Result<T, std::string>
SimpleResult<int> load_config(const std::string& path) {
    if (path.empty()) {
        return err<int, Error>("Path cannot be empty");
    }
    // ... load config
    return ok<int, Error>(42);
}

auto config = load_config("config.json")
    .unwrap_or(100);  // Default config value
```

### Real-World Example - Database Query

```cpp
using namespace ase::types;

struct User {
    uint32_t id;
    std::string name;
};

// Database function that can fail
SimpleResult<User> find_user(uint32_t id) {
    if (id == 0) {
        return err<User, Error>("Invalid user ID");
    }
    // ... query database
    if (/* user not found */) {
        return err<User, Error>("User not found");
    }
    return ok<User, Error>(User{id, "Alice"});
}

// Usage with chaining
auto result = find_user(42)
    .map<std::string>([](const User& u) { return u.name; })
    .map<std::string>([](const std::string& name) { return "Hello, " + name; });

if (result.is_ok()) {
    std::cout << result.unwrap() << std::endl;  // "Hello, Alice"
} else {
    std::cerr << "Error: " << result.unwrap_err() << std::endl;
}

// Or with default handling
std::string greeting = find_user(42)
    .map<std::string>([](const User& u) { return "Hello, " + u.name; })
    .unwrap_or("Hello, Guest");
```

### Real-World Example - File Operations

```cpp
using namespace ase::types;

SimpleResult<std::string> read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return err<std::string, Error>("Failed to open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return ok<std::string, Error>(buffer.str());
}

// Chain file operations
auto content = read_file("config.json")
    .and_then<nlohmann::json>([](const std::string& text) -> SimpleResult<nlohmann::json> {
        try {
            return ok<nlohmann::json, Error>(nlohmann::json::parse(text));
        } catch (const std::exception& e) {
            return err<nlohmann::json, Error>("JSON parse error: " + std::string(e.what()));
        }
    })
    .map<int>([](const nlohmann::json& j) { return j["port"].get<int>(); })
    .unwrap_or(8090);  // Default port
```

## API Reference

### Option<T>

| Member | Description |
|--------|-------------|
| `some(value)` | Create Option with value (static) |
| `none()` | Create empty Option (static) |
| `is_some()` | Check if contains value |
| `is_none()` | Check if empty |
| `unwrap()` | Get value or throw |
| `unwrap_or(default)` | Get value or return default |
| `unwrap_or_else(f)` | Get value or compute default |
| `map<U>(f)` | Transform value if present |
| `and_then<U>(f)` | Chain operations (flatMap) |
| `filter(predicate)` | Keep value if predicate passes |
| `operator*` | Dereference to value |
| `operator->` | Access member of value |

### Result<T, E>

| Member | Description |
|--------|-------------|
| `ok(value)` | Create successful Result (static) |
| `err(error)` | Create error Result (static) |
| `is_ok()` | Check if successful |
| `is_err()` | Check if error |
| `unwrap()` | Get value or throw |
| `unwrap_err()` | Get error or throw |
| `unwrap_or(default)` | Get value or return default |
| `unwrap_or_else(f)` | Get value or compute from error |
| `map<U>(f)` | Transform value if ok |
| `and_then<U>(f)` | Chain operations (flatMap) |
| `map_err<F>(f)` | Transform error if err |
| `operator*` | Dereference to value |
| `operator->` | Access member of value |

### Type Aliases

| Alias | Definition | Use Case |
|-------|------------|----------|
| `Error` | `std::string` | Simple error messages |
| `SimpleResult<T>` | `Result<T, Error>` | Result with string error |

## Design Philosophy

### Type Safety
Compiler enforces handling of None and Err cases - no silent failures.

### Zero Cost
Built on `std::optional` and `std::variant` - no runtime overhead compared to manual checks.

### Explicit Over Implicit
Every error path is visible in the type signature.

### Functional API
Chain operations with `map` and `and_then` for expressive error handling.

### No Exceptions
Exceptions are still available via `unwrap()`, but encouraged to use `unwrap_or` or pattern matching.

## Comparison

### Option vs std::optional

```cpp
// std::optional
std::optional<int> opt = std::nullopt;
if (opt.has_value()) {
    int val = opt.value();
}
int result = opt.value_or(0);

// ase::types::Option (same semantics, better API)
Option<int> opt = none<int>();
if (opt.is_some()) {
    int val = opt.unwrap();
}
int result = opt.unwrap_or(0);

// Bonus: Functional operations
auto doubled = opt.map<int>([](int x) { return x * 2; });
```

### Result vs Exceptions

```cpp
// Traditional exceptions (hidden control flow)
int divide(int a, int b) {
    if (b == 0) throw std::runtime_error("Division by zero");
    return a / b;
}
// No indication in signature that this can fail!

// ase::types::Result (explicit in signature)
Result<int, std::string> divide(int a, int b) {
    if (b == 0) return err<int, std::string>("Division by zero");
    return ok<int, std::string>(a / b);
}
// Signature tells you this can fail!
```

### Result vs Error Codes

```cpp
// Traditional error codes (easy to ignore)
int divide(int a, int b, int* result) {
    if (b == 0) return -1;  // Error code
    *result = a / b;
    return 0;  // Success
}
// Caller can ignore return value!

// ase::types::Result (must be handled)
Result<int, std::string> divide(int a, int b) {
    if (b == 0) return err<int, std::string>("Division by zero");
    return ok<int, std::string>(a / b);
}
// Compiler warns if you don't check is_ok() or unwrap!
```

## Dependencies

### External
- C++20 standard library (`<optional>`, `<variant>`, `<string>`, `<functional>`)

### Internal
- None (Layer 0 - Foundation)

## License

Proprietary - ASE Engine

---

**Layer 0 Foundation** | No ASE dependencies | Header-only | C++20
