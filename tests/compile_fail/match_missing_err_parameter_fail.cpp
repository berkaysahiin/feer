#include <feer/result.hpp>

int main() {
    feer::Result<int> value = feer::Err{"boom"};

    auto out = value.match(
        [](int v) {
            return v;
        },
        [] {
            return -1;
        });

    (void)out;
    return 0;
}
