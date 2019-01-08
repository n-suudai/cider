
#include "System/Log.hpp"
#include "Win32Prerequisites.hpp"
#include <string.h>
#include <cstdarg>
#include <cstdio>


namespace Cider {
namespace System {


Void Log::Format(Level level, const Char* format, ...)
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


Void Log::Message(Level level, const Char* message)
{
    Char buffer[1024];

    const Char* strLevel[Num] = {
    "Verbose",
    "Debug",
    "Info",
    "Warning",
    "Error",
    "Assert",
    };

    sprintf_s(buffer, "【%s】\n%s\n", strLevel[level], message);

    OutputDebugStringA(buffer);
}


Void Log::Format(const Char* format, ...)
{
    if (strlen(format) < 1) return;

    Char buffer[1024];
    {
        std::va_list vlist;
        va_start(vlist, format);
        _vsnprintf_s(buffer, _TRUNCATE, format, vlist);
        va_end(vlist);
    }

    Message(buffer);
}


Void Log::Message(const Char* message)
{
    OutputDebugStringA(message);
}


} // namespace System
} // namespace Cider

