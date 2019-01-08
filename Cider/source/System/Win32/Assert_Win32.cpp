
#include "System/Assert.hpp"
#include <crtdbg.h>


namespace Cider {
namespace System {


Void BreakPoint()
{
    _CrtDbgBreak();
}


} // namespace System
} // namespace Cider

