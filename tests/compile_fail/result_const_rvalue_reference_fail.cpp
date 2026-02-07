#include <feer/result.hpp>

#include <utility>

int main() {
    int value = 42;
    feer::Result<const int&&> invalid = std::move(value);
    (void)invalid;
    return 0;
}
