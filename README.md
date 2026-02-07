# feer

Small Rust-like `Result` for C++20.

Header-only library: include `feer/result.hpp` and use it directly.

## Look & Feel

Basic success/error flow with explicit branching.

```cpp
#include <feer/result.hpp>

using namespace feer;

Result<int> parse_port(const std::string& text) {
    if (text == "8080") return 8080;
    return Err("invalid port");
}

if (auto r = parse_port(input)) {
    int port = r.value();
} else {
    log_error(r.error().message);
}

if (auto r = parse_port(input)) {
    use_port(r.value());
} else {
    log_error(r.error().message);
}
```

Provide fallback values with `value_or`.

```cpp
int port = parse_port(input).value_or(3000);
```

Use `match` when you want both branches in one expression.

```cpp
int port = parse_port(input).match(
    [](int value) {
        return value;
    },
    [](const Err& err) {
        log_error(err.message);
        return 3000;
    }
);
```

Carry and mutate references.

```cpp
int counter = 10;
Result<int&> r = counter;

if (r) {
    r.value() += 1;
}
```

Use `Result<void>` for operations that only report success/failure.

```bash
Result<void> foo() {
    if(m_initialized) {
        return Ok();
    }

    return Err("Object is not initialized");
}

if (auto r = foo()) {
    start_runtime();
} else {
    log_error(r.error().message);
}
```
