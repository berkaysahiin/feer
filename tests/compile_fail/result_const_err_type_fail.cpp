#include <feer/result.hpp>

int main() {
    feer::Result<const feer::Err> invalid = feer::Err("must-fail");
    (void)invalid;
    return 0;
}
