#include <feer/result.hpp>

int main() {
    feer::Result<void> value;

    auto out = value.match(
        [](int) {
            return 1;
        },
        [](const feer::Err&) {
            return 0;
        });

    (void)out;
    return 0;
}
