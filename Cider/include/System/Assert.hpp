
#pragma once

#include "System/Types.hpp"
#include "System/DebugBreak.hpp"


#ifndef CIDER_ASSERT
#   ifdef _DEBUG
#       define CIDER_ASSERT(expression, message) \
            static_cast<Cider::Void>( \
                (!!(expression)) \
                || (Cider::System::AssertHandle(#expression, message, __FILE__, __LINE__), CIDER_DEBUG_BREAK(), false))

#   else
#       define CIDER_ASSERT(expression, message) \
            static_cast<Cider::Void>(0)
#   endif
#endif


namespace Cider {
namespace System {


Void AssertHandle(const Char* expression, const Char* message, const Char* file, Int32 line);


} // namespace System
} // namespace Cider

