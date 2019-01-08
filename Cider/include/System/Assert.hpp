
#pragma once

#include "System/Types.hpp"


#ifndef CIDER_ASSERT
#   define CIDER_ASSERT(expression, message) \
        static_cast<void>( \
            (!!(expression)) \
            || (Cider::System::AssertHandle(#expression, message, __FILE__, __LINE__), false))
#endif


namespace Cider {
namespace System {


Void BreakPoint();


Void AssertHandle(const Char* expression, const Char* message, const Char* file, Int32 line);


} // namespace System
} // namespace Cider

