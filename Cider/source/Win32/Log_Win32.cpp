
#include "Log.hpp"
#include "Win32Prerequisites.hpp"
#include <string.h>
#include <cstdarg>
#include <cstdio>


namespace Cider {
namespace Log {


Void CIDER_APIENTRY Format(Level level, const Char* format, ...)
{
    if (strlen(format) < 1) return;

    Char buffer[1024];
    {
        std::va_list vlist;
        va_start(vlist, format);
        _vsnprintf_s(buffer, _TRUNCATE, format, vlist);
        va_end(vlist);
    }

    Message(level, buffer);
}


Void CIDER_APIENTRY Message(Level level, const Char* message)
{
    Char buffer[1024];

    const Char* strLevel[static_cast<Int32>(Level::Num)] = {
    "Verbose",
    "Debug",
    "Info",
    "Warning",
    "Error",
    "Assert",
    };

    sprintf_s(buffer, "【%s】\n%s\n", strLevel[static_cast<Int32>(level)], message);

    OutputDebugStringA(buffer);
}


} // namespace Log
} // namespace Cider

