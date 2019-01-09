
#pragma once

#if defined(_MSC_VER)
#include <xutility>
#endif


#ifndef CIDER_DEBUG_BREAK
#   if defined(_MSC_VER)
#       define CIDER_DEBUG_BREAK() _CrtDbgBreak()
#   else
#       define CIDER_DEBUG_BREAK() asm("int $3")
#   endif
#endif

