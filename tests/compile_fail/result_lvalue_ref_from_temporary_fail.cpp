#include <feer/result.hpp>

int main() {
    feer::Result<int&> invalid = 123;
    (void)invalid;
    return 0;
}
