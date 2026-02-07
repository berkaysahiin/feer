#include <feer/result.hpp>

int main() {
    feer::Result<feer::Err> invalid = feer::Err("must-fail");
    (void)invalid;
    return 0;
}
