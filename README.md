# feer

Small Rust-like `Result` for C++20.

## Look & Feel

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
```

```cpp
int port = parse_port(input).value_or(3000);
```

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

```cpp
int counter = 10;
Result<int&> r = counter;

if (r) {
    r.value() += 1;
}
```

## Build

```bash
./scripts/build.sh
./scripts/build_w_test.sh
```

Optional build directory:

```bash
./scripts/build.sh out
./scripts/build_w_test.sh out
```
