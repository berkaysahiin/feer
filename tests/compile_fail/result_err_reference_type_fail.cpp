#include <feer/result.hpp>

int main() {
    feer::Err err{"must-fail"};
    feer::Result<feer::Err&> invalid = err;
    (void)invalid;
    return 0;
}
