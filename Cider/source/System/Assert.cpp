
#include "System/Assert.hpp"
#include "System/Log.hpp"
#include "System/StackTrace.hpp"


namespace Cider {
namespace System {


Void AssertHandle(const Char* expression, const Char* message, const Char* file, Int32 line)
{
    Log::Format(
        Log::Assert,
        "%s(%d) : { expression=\"%s\" }\n%s",
        file,
        line,
        expression,
        message
    );

    constexpr UInt32 TRACE_INFO_BUFFER_COUNT = 62;

    static StackTrace::TraceInfo s_traceInfo[TRACE_INFO_BUFFER_COUNT];

    UInt32 captureCount = StackTrace::CaptureStackTrace(
        s_traceInfo,
        TRACE_INFO_BUFFER_COUNT
    );

    if (captureCount > 0)
    {
        Log::Message(
            "========================================\n"
        );

        for (UInt32 i = 0; i < captureCount; ++i)
        {
            s_traceInfo[i].Print();
        }

        Log::Message(
            "========================================\n"
        );
    }

    BreakPoint();
}


} // namespace System
} // namespace Cider

