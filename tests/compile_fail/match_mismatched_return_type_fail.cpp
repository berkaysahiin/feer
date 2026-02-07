#include <feer/result.hpp>

int main() {
    feer::Result<int> value = 7;

    auto out = value.match(
        [](int v) {
            return v;
        },
        [](const feer::Err&) {
            return false;
        });

    (void)out;
    return 0;
}
