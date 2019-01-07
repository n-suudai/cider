
#include "Cider.hpp"
#include <string.h>
#include <cstdarg>
#include <cstdio>


void CIDER_APIENTRY Cider_Hello()
{
    Cider::Hello();
}


void CIDER_APIENTRY Cider_LogFormat(int level, const char* format, ...)
{
    if (strlen(format) < 1) return;

    Cider::Char buffer[1024];
    {
        std::va_list vlist;
        va_start(vlist, format);
        _vsnprintf_s(buffer, _TRUNCATE, format, vlist);
        va_end(vlist);
    }

    Cider::Log::Message(
        static_cast<Cider::Log::Level>(level),
        buffer
    );
}

void CIDER_APIENTRY Cider_LogMessage(int level, const char* message)
{
    if (strlen(message) < 1) return;

    Cider::Log::Message(
        static_cast<Cider::Log::Level>(level),
        message
    );
}


