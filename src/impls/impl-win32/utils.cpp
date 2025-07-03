#include "win32.h"
#include "utils.h"

namespace Win32 {

Error fromGetLastError() {
    return Error::unknown("win32 error");
}

} // namespace Win32
