#include <feer/result.hpp>

#include <utility>

int main() {
    int value = 7;
    feer::Result<int&&> invalid = std::move(value);
    (void)invalid;
    return 0;
}
